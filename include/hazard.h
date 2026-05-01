/*
 * hazard.h — Hazard detection and resolution interface
 *
 * Detects RAW data hazards and control hazards;
 * decides whether to forward, stall, or flush.
 * (Implementation will be done in a future stage)
 */

#ifndef HAZARD_H
#define HAZARD_H

#include "../include/types.h"

/*
 * Check for data hazards between the instruction in EX and
 * the instruction in ID. Returns 1 if a stall is needed, 0 otherwise.
 * Also sets up forwarding values if applicable.
 */
/* int hazard_detect(const ID_EX_Reg *id_ex, const IF_ID_Reg *if_id); */

#endif /* HAZARD_H */
