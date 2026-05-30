MAX_FREQ_HZ = 420_000_000
MAX_AMPLITUDE = 16_383
MAX_PHASE_DEG = 360


def _parse_uint(name, value, limit):
    if not value.isdigit():
        raise ValueError(f"{name} must be unsigned integer")
    parsed = int(value)
    if parsed > limit:
        raise ValueError(f"{name} out of range")
    return parsed


def parse_command(line):
    text = line.strip()
    if not text:
        raise ValueError("empty command")

    upper = text.upper()
    if upper in ("DDS?", "SINGLE?"):
        return {"type": "query"}

    normalized = upper.replace(",", " ")
    tokens = [token for token in normalized.split() if token]
    if tokens and tokens[0] in ("DDS", "SINGLE", "S"):
        tokens = tokens[1:]

    result = {"type": "set", "freq_hz": None, "amplitude": None, "phase_deg": None}
    seen = False

    for token in tokens:
        if "=" not in token:
            raise ValueError("expected KEY=VALUE")
        key, value = token.split("=", 1)
        if key in ("F", "FREQ", "FREQUENCY"):
            result["freq_hz"] = _parse_uint("frequency", value, MAX_FREQ_HZ)
        elif key in ("A", "AMP", "AMPLITUDE"):
            result["amplitude"] = _parse_uint("amplitude", value, MAX_AMPLITUDE)
        elif key in ("P", "PH", "PHA", "PHASE"):
            result["phase_deg"] = _parse_uint("phase", value, MAX_PHASE_DEG)
        else:
            raise ValueError("unknown key")
        seen = True

    if not seen:
        raise ValueError("no fields")

    return result
