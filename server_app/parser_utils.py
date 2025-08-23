def parse_input(user_input):
    tokens = user_input.split()
    if not tokens:
        return None
    cmd = tokens[0]

    if cmd == "play":
        starttime = "0"
        endtime = "0"
        d = "0"
        dd = "0"
        i = 1
        while i < len(tokens):
            token = tokens[i]
            if token == "-ss":
                if i + 1 < len(tokens):
                    starttime = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -ss flag in play command")
                    return None
            elif token == "-end":
                if i + 1 < len(tokens):
                    endtime = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -end flag in play command")
                    return None
            elif token == "-d":
                if i + 1 < len(tokens):
                    d = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -d flag in play command")
                    return None
            elif token == "-dd":
                if i + 1 < len(tokens):
                    dd = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -dd flag in play command")
                    return None
            else:
                print(f"Invalid flag '{token}' in play command")
                return None
        return f"play -ss{starttime} -end{endtime} -d{d} -dd{dd}"
    elif cmd in ["pause", "stop"]:
        if len(tokens) != 1:
            print(f"Usage: {cmd}")
            return None
        return cmd
    elif cmd == "parttest":
        # Defaults
        channel = 0
        r, g, b = 255, 255, 255
        i = 1
        n = len(tokens)
        while i < n:
            token = tokens[i]
            if token == "-c":
                if i + 1 >= n:
                    print("Missing parameter for -c flag in parttest command")
                    return None
                try: 
                    channel = int(tokens[i + 1])
                except ValueError:
                    print("Channel must be integer")
                    return None
                i += 2
            elif token == "-rgb":
                if i + 3 >= n:
                    print("Missing R G B values after -rgb (need three integers)")
                    return None
                try:
                    r = int(tokens[i + 1])
                    g = int(tokens[i + 2])
                    b = int(tokens[i + 3])
                except ValueError:
                    print("RGB values must be integers")
                    return None
                # Optional range clamp
                for val in (r, g, b):
                    if not (0 <= val <= 255):
                        print("RGB values must be 0..255")
                        return None
                i += 4
            else:
                print(f"Invalid flag '{token}' in parttest command")
                return None
        return f"parttest -c {channel} -rgb {r} {g} {b}"
    else:
        print("Not support")
        return None
