from enum import Enum


class VelocityType(Enum):
    Unknown = "Unknown"
    Interval = "Vint"
    RMS = "Vrms"
    Avg = "Vavg"
    Delta = "Delta"
    Epsilon = "Epsilon"
    Eta = "Eta"
