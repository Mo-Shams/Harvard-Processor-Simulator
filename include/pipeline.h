/*
 * pipeline.h — Pipeline simulation interface
 *
 * Simulates the 3-stage pipeline (IF → ID → EX) cycle by cycle.
 */

#ifndef PIPELINE_H
#define PIPELINE_H

/*
 * Run the pipeline simulation from start to finish.
 * Assumes instruction memory has already been loaded via the parser.
 *
 * Simulates cycles until the pipeline is fully drained
 * and no more instructions remain to fetch.
 */
typedef struct {
  int total_cycles;
  int retired_instructions;
  int data_hazard_stalls;
  int branch_flushes;
} PipelineMetrics;

void pipeline_run(void);
PipelineMetrics pipeline_get_metrics(void);
void pipeline_set_verbose(int verbose);

#endif /* PIPELINE_H */
