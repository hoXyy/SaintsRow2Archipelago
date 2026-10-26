TAXI_DIVERSION_LOCATIONS = [f"Taxi Diversion - Level {i + 1}" for i in range(10)]
TAXI_DIVERSION_IDS = {
    **{name: 1000 + i for i, name in enumerate(TAXI_DIVERSION_LOCATIONS)}
}

PARAMEDIC_DIVERSION_LOCATIONS = [
    f"Paramedic Diversion - Level {i + 1}" for i in range(10)
]
PARAMEDIC_DIVERSION_IDS = {
    **{name: 1010 + i for i, name in enumerate(PARAMEDIC_DIVERSION_LOCATIONS)}
}

HOING_DIVERSION_LOCATIONS = [f"Ho-ing Diversion - Level {i + 1}" for i in range(10)]
HOING_DIVERSION_IDS = {
    **{name: 1020 + i for i, name in enumerate(HOING_DIVERSION_LOCATIONS)}
}

FIREFIGHTER_DIVERSION_LOCATIONS = [
    f"Firefighter Diversion - Level {i + 1}" for i in range(10)
]
FIREFIGHTER_DIVERSION_IDS = {
    **{name: 1030 + i for i, name in enumerate(FIREFIGHTER_DIVERSION_LOCATIONS)}
}

TOWING_DIVERSION_LOCATIONS = [f"Towing Diversion - Level {i + 1}" for i in range(10)]
TOWING_DIVERSION_IDS = {
    **{name: 1040 + i for i, name in enumerate(TOWING_DIVERSION_LOCATIONS)}
}
