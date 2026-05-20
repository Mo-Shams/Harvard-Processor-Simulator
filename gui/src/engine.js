// ============================================================
// Harvard Pipeline Simulator Engine (Package 4) - Single File
// ============================================================

const INSTR_MEM_SIZE = 1024;
const DATA_MEM_SIZE = 2048;
const NUM_GPRS = 64;
const OP = { ADD:0,SUB:1,MUL:2,LDI:3,BEQZ:4,AND:5,OR:6,JR:7,SAL:8,SAR:9,LB:10,SB:11 };
const OP_NAMES = ["ADD","SUB","MUL","LDI","BEQZ","AND","OR","JR","SAL","SAR","LB","SB"];
const R_TYPE_OPS = new Set([OP.ADD,OP.SUB,OP.MUL,OP.AND,OP.OR,OP.JR]);

// --- State (single instances, mutated in place) ---
const gprs = new Int8Array(NUM_GPRS);
const dmem = new Int8Array(DATA_MEM_SIZE);
const imem = new Uint16Array(INSTR_MEM_SIZE);
let sreg = 0, pc = 0, imem_count = 0;

function initState() {
  gprs.fill(0); dmem.fill(0); imem.fill(0);
  sreg = 0; pc = 0; imem_count = 0;
}

// --- Accessors ---
function getPC(){ return pc; }
function setPC(v){ pc = v; }
function incPC(){ pc++; }
function getSREG(){ return sreg; }
function sregGet(bit){ return (sreg>>bit)&1; }
function sregSet(bit,v){ if(v) sreg|=(1<<bit); else sreg&=~(1<<bit); sreg&=0x1F; }

// --- Decoder ---
function signExt6(v){ v&=0x3F; return (v&0x20) ? (v|0xC0)<<24>>24 : v; }
function decode(raw){
  let op=(raw>>12)&0xF, r1=(raw>>6)&0x3F, r2=raw&0x3F, imm=signExt6(raw&0x3F);
  return {op,r1,r2,imm,fmt:R_TYPE_OPS.has(op)?'R':'I'};
}

// --- ALU ---
function alu(op,r1v,r2v,imm){
  let a=r1v<<24>>24, b=r2v<<24>>24, im=imm<<24>>24, res=0;
  switch(op){
    case OP.ADD: res=a+b; break;
    case OP.SUB: res=a-b; break;
    case OP.MUL: res=a*b; break;
    case OP.LDI: res=im; break;
    case OP.AND: res=a&b; break;
    case OP.OR:  res=a|b; break;
    case OP.SAL: res=a<<im; break;
    case OP.SAR: res=a>>im; break;
    case OP.LB: case OP.SB: res=im; break;
    default: res=0;
  }
  res = res<<24>>24;
  if(op===OP.ADD){
    sregSet(4,(((r1v&0xFF)+(r2v&0xFF))&0x100)?1:0);
    let V=((a>0&&b>0&&res<0)||(a<0&&b<0&&res>0))?1:0;
    sregSet(3,V); let N=(res&0x80)?1:0; sregSet(2,N); sregSet(1,N^V); sregSet(0,res===0?1:0);
  } else if(op===OP.SUB){
    let V=((a>=0&&b<0&&res<0)||(a<0&&b>=0&&res>0))?1:0;
    sregSet(3,V); let N=(res&0x80)?1:0; sregSet(2,N); sregSet(1,N^V); sregSet(0,res===0?1:0);
  } else if([OP.MUL,OP.AND,OP.OR,OP.SAL,OP.SAR].includes(op)){
    sregSet(2,(res&0x80)?1:0); sregSet(0,res===0?1:0);
  }
  return res;
}

// --- Parser ---
function parseAssembly(text){
  let lines=text.split('\n'), addr=0, errors=[], parsed=[];
  for(let i=0;i<lines.length;i++){
    let line=lines[i].trim();
    if(!line||line[0]==='#'||line[0]===';') continue;
    let toks=line.replace(/,/g,' ').split(/\s+/).filter(t=>t);
    if(toks.length<3){ errors.push(`Line ${i+1}: need 3 tokens`); continue; }
    let opIdx=OP_NAMES.indexOf(toks[0].toUpperCase());
    if(opIdx<0){ errors.push(`Line ${i+1}: unknown '${toks[0]}'`); continue; }
    let r1Tok=toks[1].toUpperCase();
    if(!r1Tok.startsWith('R')){ errors.push(`Line ${i+1}: bad register`); continue; }
    let r1=parseInt(r1Tok.slice(1)); if(isNaN(r1)||r1<0||r1>63){ errors.push(`Line ${i+1}: bad reg`); continue; }
    let r2imm=0, isI=!R_TYPE_OPS.has(opIdx);
    if(isI){ r2imm=parseInt(toks[2]); if(isNaN(r2imm)){ errors.push(`Line ${i+1}: bad imm`); continue; } }
    else { let t=toks[2].toUpperCase(); if(!t.startsWith('R')){ errors.push(`Line ${i+1}: bad reg2`); continue; }
      r2imm=parseInt(t.slice(1)); if(isNaN(r2imm)||r2imm<0||r2imm>63){ errors.push(`Line ${i+1}: bad reg2`); continue; } }
    let enc=((opIdx&0xF)<<12)|((r1&0x3F)<<6)|(r2imm&0x3F);
    imem[addr]=enc;
    parsed.push({addr,text:line.trim(),enc});
    addr++;
  }
  imem_count=addr;
  return {count:addr,errors,parsed};
}

// --- Pipeline Engine ---
class Pipeline {
  constructor(){
    this.if_id={valid:false,pc:0,instr:0};
    this.id_ex={valid:false,pc:0,op:0,r1:0,r2:0,r1v:0,r2v:0,imm:0};
    this.cycle=0; this.fetchDone=false; this.done=false; this.history=[];
  }
  
  step(){
    if(this.done) return null;
    this.cycle++;
    let snap={cycle:this.cycle, IF:{},ID:{},EX:{}, changes:{regs:[],mem:[],sreg:null,pc:null}, hazard:null, done:false};
    
    let oldGprs=new Int8Array(gprs), oldDmem=new Int8Array(dmem), oldSreg=sreg, oldPc=pc;
    let ifSnap={...this.if_id}, exSnap={...this.id_ex};
    let exWasValid=this.id_ex.valid, exOp=this.id_ex.op, exR1=this.id_ex.r1;

    // === EX ===
    let flush=false;
    if(exSnap.valid){
      let d=decode(imem[exSnap.pc]);
      switch(exSnap.op){
        case OP.ADD:case OP.SUB:case OP.MUL:case OP.AND:case OP.OR:case OP.SAL:case OP.SAR:{
          let res=alu(exSnap.op,exSnap.r1v,exSnap.r2v,exSnap.imm);
          gprs[exSnap.r1]=res;
          snap.EX={valid:true,pc:exSnap.pc,name:OP_NAMES[exSnap.op],d,r1v:exSnap.r1v,r2v:exSnap.r2v,imm:exSnap.imm,
            out:`R${exSnap.r1} = ${res}`};
          break;}
        case OP.LDI:{
          let res=alu(OP.LDI,exSnap.r1v,exSnap.r2v,exSnap.imm);
          gprs[exSnap.r1]=res;
          snap.EX={valid:true,pc:exSnap.pc,name:"LDI",d,r1v:exSnap.r1v,imm:exSnap.imm,
            out:`R${exSnap.r1} = ${res}`};
          break;}
        case OP.LB:{
          let addr=(exSnap.imm<<24>>24)&0xFFFF;
          if(addr>=DATA_MEM_SIZE) addr=0;
          let loaded=dmem[addr]; gprs[exSnap.r1]=loaded;
          snap.EX={valid:true,pc:exSnap.pc,name:"LB",d,r1v:exSnap.r1v,imm:exSnap.imm,
            out:`R${exSnap.r1} = Mem[${addr}] = ${loaded}`};
          break;}
        case OP.SB:{
          let addr=(exSnap.imm<<24>>24)&0xFFFF;
          if(addr>=DATA_MEM_SIZE) addr=0;
          dmem[addr]=exSnap.r1v;
          snap.EX={valid:true,pc:exSnap.pc,name:"SB",d,r1v:exSnap.r1v,imm:exSnap.imm,
            out:`Mem[${addr}] = ${exSnap.r1v}`};
          break;}
        case OP.BEQZ:{
          if(exSnap.r1v===0){
            let target=(exSnap.pc+1+exSnap.imm)&0xFFFF; pc=target; flush=true;
            snap.EX={valid:true,pc:exSnap.pc,name:"BEQZ",d,r1v:exSnap.r1v,imm:exSnap.imm,
              out:`Branch TAKEN → PC=${target}`};
          } else {
            snap.EX={valid:true,pc:exSnap.pc,name:"BEQZ",d,r1v:exSnap.r1v,imm:exSnap.imm,
              out:`NOT taken (R${exSnap.r1}=${exSnap.r1v})`};
          }
          break;}
        case OP.JR:{
          let target=((exSnap.r1v&0xFF)<<8)|(exSnap.r2v&0xFF); pc=target; flush=true;
          snap.EX={valid:true,pc:exSnap.pc,name:"JR",d,r1v:exSnap.r1v,r2v:exSnap.r2v,
            out:`Jump → PC=${target}`};
          break;}
        default: snap.EX={valid:false};
      }
    } else { snap.EX={valid:false}; }

    // Detect changes from EX
    for(let i=0;i<NUM_GPRS;i++) if(gprs[i]!==oldGprs[i]) snap.changes.regs.push({i,from:oldGprs[i],to:gprs[i],stage:'EX'});
    for(let i=0;i<DATA_MEM_SIZE;i++) if(dmem[i]!==oldDmem[i]) snap.changes.mem.push({i,from:oldDmem[i],to:dmem[i],stage:'EX'});
    if(sreg!==oldSreg) snap.changes.sreg={from:oldSreg,to:sreg};
    if(pc!==oldPc) snap.changes.pc={from:oldPc,to:pc,stage:'EX'};

    // === HAZARD ===
    // No stalls needed: in this 3-stage pipeline, EX runs before ID
    // (reverse order), so all forwarding is implicit — including LB.
    let stall=false;
    if(flush) snap.hazard={type:'flush',reason:'branch/jump taken'};

    // === ID ===
    if(flush){
      this.id_ex.valid=false;
      snap.ID=ifSnap.valid?{valid:true,pc:ifSnap.pc,status:'FLUSHED',d:decode(ifSnap.instr)}:{valid:false};
    } else if(stall){
      this.id_ex.valid=false;
      snap.ID={valid:true,pc:ifSnap.pc,status:'STALLED',d:decode(ifSnap.instr)};
    } else if(ifSnap.valid){
      let d=decode(ifSnap.instr);
      this.id_ex={valid:true,pc:ifSnap.pc,op:d.op,r1:d.r1,r2:d.r2,r1v:gprs[d.r1],r2v:gprs[d.r2],imm:d.imm};
      snap.ID={valid:true,pc:ifSnap.pc,status:'OK',d,r1v:this.id_ex.r1v,r2v:this.id_ex.r2v};
    } else {
      this.id_ex.valid=false; snap.ID={valid:false};
    }

    // === IF ===
    let pcBefore=pc;
    if(flush){
      this.if_id.valid=false; this.fetchDone=false;
      snap.IF={valid:false,status:'FLUSHED'};
    } else if(stall){
      snap.IF={valid:this.if_id.valid,pc:this.if_id.pc,status:'STALLED'};
    } else if(pc<imem_count){
      this.if_id={valid:true,pc:pc,instr:imem[pc]}; pc++;
      snap.IF={valid:true,pc:this.if_id.pc,raw:this.if_id.instr,d:decode(this.if_id.instr),status:'OK'};
      if(pc!==pcBefore) snap.changes.pc={from:pcBefore,to:pc,stage:'IF'};
    } else {
      this.if_id.valid=false; this.fetchDone=true;
      snap.IF={valid:false,status:'DONE'};
    }

    if(!this.if_id.valid && !this.id_ex.valid && this.fetchDone) this.done=true;
    snap.done=this.done;
    this.history.push(snap);
    return snap;
  }
}

export { initState, parseAssembly, Pipeline, NUM_GPRS, DATA_MEM_SIZE, INSTR_MEM_SIZE,
         OP_NAMES, R_TYPE_OPS, OP, gprs, dmem, imem, getPC, getSREG, sregGet, decode };
