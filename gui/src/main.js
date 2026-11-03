import { initState, parseAssembly, Pipeline, NUM_GPRS, DATA_MEM_SIZE, INSTR_MEM_SIZE,
         OP_NAMES, R_TYPE_OPS, OP, gprs, dmem, imem, getPC, getSREG, sregGet, decode } from './engine.js';

let engine = null, autoPlay = null, parsedLines = [];
const $ = id => document.getElementById(id);

// DOM
const codeEl=$('code-editor'), lineNums=$('line-numbers'), errBox=$('parse-errors'),
      consoleEl=$('console-log'), cycleEl=$('cycle-display'), pcEl=$('val-pc'),
      gprGrid=$('gpr-grid'), dmemGrid=$('dmem-grid'), imemGrid=$('imem-grid'),
      btnParse=$('btn-assemble'), btnReset=$('btn-reset'), btnPlay=$('btn-play'),
      btnStep=$('btn-step'), speedEl=$('speed-slider'), progSelect=$('program-select'),
      tabDmem=$('tab-dmem'), tabImem=$('tab-imem'),
      metricCycles=$('metric-cycles'), metricRetired=$('metric-retired'),
      metricCpi=$('metric-cpi'), metricStalls=$('metric-stalls'),
      metricFlushes=$('metric-flushes'), timingEl=$('timing-diagram');

const PRESETS = {
  test_comprehensive: `LDI R1 10
LDI R2 3
LDI R3 10
ADD R3 R2
LDI R4 10
SUB R4 R2
LDI R5 10
MUL R5 R2
LDI R6 10
AND R6 R2
LDI R7 10
OR R7 R2
LDI R8 10
SAL R8 2
LDI R9 10
SAR R9 1
SB R5 5
LB R11 5
BEQZ R10 1
LDI R12 1
LDI R10 0
BEQZ R1 1
LDI R20 7`,
  test1: `LDI R1 5
LDI R2 3
ADD R1 R2
SUB R1 R2
MUL R1 R2
AND R1 R2
OR R1 R2
SAL R1 2
SAR R1 1
SB R1 0
LB R3 0`,
  test_hazard_flush: `LDI R1 0
LDI R2 5
LB R3 0
ADD R3 R2
BEQZ R1 1
LDI R4 31
LDI R5 7`,
  test_jr: `LDI R1 0
LDI R2 5
JR R1 R2
LDI R3 9
LDI R4 9
LDI R5 7`,
  test_loop: `LDI R1 4
LDI R2 0
LDI R3 1
ADD R2 R1
SUB R1 R3
BEQZ R1 1
BEQZ R0 -4
LDI R4 20`,
  test_memory_array: `LDI R1 10
SB R1 0
LDI R1 20
SB R1 1
LDI R1 30
SB R1 2
LB R10 0
LB R11 1
LB R12 2
ADD R10 R11
ADD R10 R12`
};

// ═══ INIT ═══
function init(){
  codeEl.addEventListener('input', updateLines);
  codeEl.addEventListener('scroll', ()=> lineNums.scrollTop=codeEl.scrollTop);
  updateLines();
  gprGrid.innerHTML='';
  for(let i=0;i<NUM_GPRS;i++){
    const d=document.createElement('div'); d.className='gpr-cell'; d.id=`gpr-${i}`;
    d.innerHTML=`<span class="rn">R${i}</span><span class="rv" id="rv-${i}">0</span>`;
    gprGrid.appendChild(d);
  }
  tabDmem.addEventListener('click',()=>{tabDmem.classList.add('active');tabImem.classList.remove('active');dmemGrid.classList.remove('hidden');imemGrid.classList.add('hidden');});
  tabImem.addEventListener('click',()=>{tabImem.classList.add('active');tabDmem.classList.remove('active');imemGrid.classList.remove('hidden');dmemGrid.classList.add('hidden');});
  resetAll();
}

function updateLines(){
  const n=codeEl.value.split('\n').length;
  lineNums.innerHTML=Array.from({length:n},(_,i)=>i+1).join('<br>');
}

function log(msg,cls=''){
  const d=document.createElement('div'); d.className=`log-line ${cls}`; d.textContent=msg;
  consoleEl.appendChild(d); consoleEl.scrollTop=consoleEl.scrollHeight;
}

// ═══ RESET ═══
function resetAll(){
  stopAuto(); initState(); engine=null; parsedLines=[];
  consoleEl.innerHTML=''; log('Ready. Write assembly and click Parse & Load.','muted');
  cycleEl.textContent='Cycle 0'; pcEl.textContent='0';
  metricCycles.textContent='0'; metricRetired.textContent='0';
  metricCpi.textContent='—'; metricStalls.textContent='0'; metricFlushes.textContent='0';
  timingEl.innerHTML='<span class="muted">Parse a program to see timing.</span>';
  for(let i=0;i<NUM_GPRS;i++){
    const rv=$(`rv-${i}`); if(rv) rv.textContent='0';
    const cell=$(`gpr-${i}`); if(cell) cell.classList.remove('changed');
  }
  [0,1,2,3,4].forEach(b=>{const el=$(`flag-${['z','s','n','v','c'][b]}`);if(el)el.classList.remove('on');});
  updateCard('card-if',null); updateCard('card-id',null); updateCard('card-ex',null);
  dmemGrid.innerHTML='<div class="muted" style="padding:0.8rem">No data written yet</div>';
  imemGrid.innerHTML='<div class="muted" style="padding:0.8rem">No program loaded</div>';
  errBox.classList.add('hidden'); codeEl.disabled=false;
  btnStep.disabled=true; btnPlay.disabled=true;
}

// ═══ PARSE ═══
function doParse(){
  resetAll();
  const res=parseAssembly(codeEl.value);
  if(res.errors.length){errBox.textContent=res.errors.join('\n');errBox.classList.remove('hidden');log('Parse failed.','err');return;}
  errBox.classList.add('hidden'); parsedLines=res.parsed; engine=new Pipeline();
  log(`✓ Parsed ${res.count} instructions.`,'ok');
  codeEl.disabled=true; btnStep.disabled=false; btnPlay.disabled=false;
  updateImemGrid();
}

// ═══ FORMAT ═══
function fmtInstr(d){
  if(!d) return '?';
  const n=OP_NAMES[d.op]||'?';
  return d.fmt==='R'?`${n} R${d.r1} R${d.r2}`:`${n} R${d.r1} ${d.imm}`;
}

function fmtInputs(d,r1v,r2v,imm){
  if(!d) return '';
  if(d.fmt==='R') return `R${d.r1}=${r1v}, R${d.r2}=${r2v}`;
  return `R${d.r1}=${r1v}, IMM=${imm}`;
}

// ═══ STAGE CARDS ═══
function updateCard(id,data){
  const card=$(id);
  card.className='stage-card';
  const nameEl=card.querySelector('.iname'), detEl=card.querySelector('.idetail'), inputEl=card.querySelector('.iinput');
  if(!data||!data.valid){
    if(data&&data.status==='FLUSHED'){card.classList.add('flushed');nameEl.textContent='✕ FLUSHED';detEl.textContent='';inputEl.textContent='';}
    else{card.classList.add('bubble');nameEl.textContent='Bubble';detEl.textContent='—';inputEl.textContent='';}
    return;
  }
  card.classList.add('active');
  nameEl.textContent=`[${data.pc}] ${fmtInstr(data.d)}`;
  if(data.status==='STALLED'){card.classList.add('stalled');detEl.textContent='⚠ STALLED (load-use)';inputEl.textContent='';return;}
  if(data.status==='FORWARDED'){card.classList.add('forwarded');detEl.textContent='↗ FORWARDED from EX';}
  // Inputs line
  if(data.r1v!==undefined) inputEl.textContent='In: '+fmtInputs(data.d,data.r1v,data.r2v,data.imm||data.d.imm);
  else inputEl.textContent='';
  // Output line
  detEl.textContent=data.out?'Out: '+data.out:'';
}

function renderTiming(){
  if(!engine || !engine.history.length) return;
  const rows=engine.history.filter(x=>x.EX.valid||x.ID.valid||x.IF.valid)
    .map(x=>`<div class="timing-row"><span class="timing-name">C${x.cycle}</span>`+
      `<span class="timing-cell ${x.IF.valid?'if':''}">${x.IF.valid?'F':x.IF.status==='FLUSHED'?'×':''}</span>`+
      `<span class="timing-cell ${x.ID.valid?'id':''}">${x.ID.valid?'D':x.ID.status==='FLUSHED'?'×':''}</span>`+
      `<span class="timing-cell ${x.EX.valid?'ex':''}">${x.EX.valid?'E':''}</span></div>`).join('');
  timingEl.innerHTML=rows || '<span class="muted">No instructions retired.</span>';
}

// ═══ STEP ═══
function doStep(){
  if(!engine||engine.done) return;
  const s=engine.step();
  if(!s) return;

  document.querySelectorAll('.changed').forEach(e=>e.classList.remove('changed'));

  updateCard('card-ex',s.EX);
  updateCard('card-id',s.ID);
  updateCard('card-if',s.IF);

  cycleEl.textContent=`Cycle ${s.cycle}`;
  metricCycles.textContent=s.metrics.cycles;
  metricRetired.textContent=s.metrics.retired;
  metricCpi.textContent=s.metrics.cpi.toFixed(2);
  metricStalls.textContent=s.metrics.stalls;
  metricFlushes.textContent=s.metrics.flushes;
  renderTiming();
  if(s.changes.pc) pcEl.textContent=s.changes.pc.to;

  // SREG
  ['c','v','n','s','z'].forEach((f,i)=>{$(`flag-${f}`).classList.toggle('on',sregGet(4-i));});

  // GPR
  s.changes.regs.forEach(ch=>{
    const rv=$(`rv-${ch.i}`); if(rv) rv.textContent=ch.to;
    const cell=$(`gpr-${ch.i}`); if(cell) cell.classList.add('changed');
  });

  // Memory
  if(s.changes.mem.length) updateDmemGrid();

  // Console
  log(`══ Cycle ${s.cycle} ══`,'cycle');
  if(s.EX.valid){
    log(`  [EX] ${s.EX.name} @ PC=${s.EX.pc}`);
    log(`       Inputs: ${fmtInputs(s.EX.d,s.EX.r1v,s.EX.r2v,s.EX.imm)}`);
    log(`       Output: ${s.EX.out}`);
  } else log('  [EX] bubble');
  if(s.ID.valid){
    log(`  [ID] ${fmtInstr(s.ID.d)} @ PC=${s.ID.pc} ${s.ID.status!=='OK'?'→ '+s.ID.status:''}`);
    if(s.ID.status==='OK'||s.ID.status==='FORWARDED') log(`       Decoded: ${fmtInputs(s.ID.d,s.ID.r1v,s.ID.r2v,s.ID.d.imm)}`);
  } else log('  [ID] bubble');
  if(s.IF.valid) log(`  [IF] Fetched PC=${s.IF.pc}: ${fmtInstr(s.IF.d)} (0x${s.IF.raw.toString(16).padStart(4,'0').toUpperCase()})`);
  else if(s.IF.status==='FLUSHED') log('  [IF] → FLUSHED');
  else if(s.IF.status==='STALLED') log('  [IF] → STALLED');
  else log('  [IF] no more instructions');

  if(s.hazard) log(`  ⚠ ${s.hazard.type.toUpperCase()}: ${s.hazard.reason}`,s.hazard.type==='flush'?'err':'warn');
  s.changes.regs.forEach(ch=>log(`  [EX] R${ch.i}: ${ch.from} → ${ch.to}`,'ok'));
  s.changes.mem.forEach(ch=>log(`  [EX] Mem[${ch.i}]: ${ch.from} → ${ch.to}`,'ok'));
  if(s.changes.sreg) log(`  [EX] SREG: 0x${s.changes.sreg.from.toString(16).padStart(2,'0')} → 0x${s.changes.sreg.to.toString(16).padStart(2,'0')} (C=${sregGet(4)} V=${sregGet(3)} N=${sregGet(2)} S=${sregGet(1)} Z=${sregGet(0)})`,'ok');
  if(s.changes.pc) log(`  [${s.changes.pc.stage}] PC: ${s.changes.pc.from} → ${s.changes.pc.to}`);

  if(s.done){
    log('');
    log('════════ PIPELINE COMPLETE ════════','cycle');
    log(`Total cycles: ${s.cycle}`);
    log('');
    // Final state dump
    log('── Registers ──','cycle');
    let regLine='';
    for(let i=0;i<NUM_GPRS;i++){
      regLine+=`R${i}:${gprs[i]} `;
      if((i+1)%8===0){log('  '+regLine.trim()); regLine='';}
    }

    log(`  SREG=0x${getSREG().toString(16).padStart(2,'0')} C=${sregGet(4)} V=${sregGet(3)} N=${sregGet(2)} S=${sregGet(1)} Z=${sregGet(0)}`);
    log(`  PC=${getPC()}`);
    log('');
    log('── Data Memory (non-zero) ──','cycle');
    let anyMem=false;
    for(let i=0;i<DATA_MEM_SIZE;i++){if(dmem[i]!==0){log(`  [${i}] = ${dmem[i]}`);anyMem=true;}}
    if(!anyMem) log('  (all zeros)');

    stopAuto(); btnStep.disabled=true; btnPlay.disabled=true;
  }
}

// ═══ MEMORY GRIDS ═══
function updateDmemGrid(){
  dmemGrid.innerHTML='';
  let any=false;
  for(let i=0;i<DATA_MEM_SIZE;i++){
    if(dmem[i]!==0){
      any=true;
      const d=document.createElement('div'); d.className='mem-cell'; d.id=`mem-${i}`;
      d.innerHTML=`<span class="addr">[${i}]</span><span class="val">${dmem[i]}</span>`;
      dmemGrid.appendChild(d);
    }
  }
  if(!any) dmemGrid.innerHTML='<div class="muted" style="padding:0.8rem">All zeros</div>';
}

function updateImemGrid(){
  imemGrid.innerHTML='';
  for(let i=0;i<parsedLines.length;i++){
    const p=parsedLines[i], d=decode(p.enc);
    const el=document.createElement('div'); el.className='mem-cell imem-cell';
    el.innerHTML=`<span class="addr">[${p.addr}]</span><span class="val">0x${p.enc.toString(16).padStart(4,'0').toUpperCase()}</span><span class="asm">${fmtInstr(d)}</span>`;
    imemGrid.appendChild(el);
  }
}

// ═══ AUTO PLAY ═══
function toggleAuto(){
  if(autoPlay){stopAuto();return;}
  btnPlay.querySelector('.play-icon').style.display='none';
  btnPlay.querySelector('.pause-icon').style.display='block';
  const spd=2100-parseInt(speedEl.value);
  autoPlay=setInterval(()=>{if(!engine||engine.done){stopAuto();return;}doStep();},spd);
}
function stopAuto(){
  if(autoPlay){clearInterval(autoPlay);autoPlay=null;}
  const playI=btnPlay.querySelector('.play-icon'), pauseI=btnPlay.querySelector('.pause-icon');
  if(playI) playI.style.display='block';
  if(pauseI) pauseI.style.display='none';
}

// ═══ EVENTS ═══
btnParse.addEventListener('click',doParse);
btnReset.addEventListener('click',resetAll);
btnStep.addEventListener('click',doStep);
btnPlay.addEventListener('click',toggleAuto);
speedEl.addEventListener('input',()=>{if(autoPlay){stopAuto();toggleAuto();}});
if(progSelect){
  progSelect.addEventListener('change',()=>{
    const val=progSelect.value;
    if(PRESETS[val]){
      codeEl.value=PRESETS[val];
      updateLines();
      resetAll();
    }
  });
}

init();
