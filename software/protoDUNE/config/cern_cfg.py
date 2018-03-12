from itertools import product

# =============
# Shelf-manager
# =============
host_sm = 'np04-sm-1'

# ==========================
# ATCA slot map: cob -> slot
# --------------------------
# At CERN, each RCE has a hostname np04-cobX-rceY.
# This map tell where the COBs are installed.
# format _cob_ : _slot_
# ==========================
cob_to_slot = {
    1 : 1,
    2 : 2,
}

# =========================
# Slot map: host/ip -> name 
# -------------------------
# Map host/ip to default naming scheme CERN-100, CERN-102m ....
# =========================
slot_map = { (host_sm) : 'SM-CERN' }


# ===================================
# host map: host/ip -> slot, dpm, rce
# -----------------------------------
# Map host/ip to ATCA slot
# e.g. np04-cob2-rce3 -> 2, 1, 0
# Mostly use for power reset via shelf manager
# ==================================
host_to_slot = {}

# maps generation
for cob, slot in cob_to_slot.items():
    for i, (dpm, rce) in enumerate(product(range(4), [0, 2])):
        host = 'np04-cob{}-rce{}'.format(cob, i + 1)
        slot_map[(host)] = 'CERN-{}{}{}'.format(slot, dpm, rce)
        host_to_slot[host] = (slot, dpm, rce)

    dtm_host = 'np04-cob{}-rce9'.format(cob)
    slot_map[(dtm_host)] = 'CERN-DTM{}'.format(slot)
    host_to_slot[dtm_host] = (slot, 4, 0)
