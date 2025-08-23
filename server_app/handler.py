from utils import day_micro

# handle the time synchronization messages from clients
# This function will be called in a separate thread for each client connection.
def handle_client(client, addr, clients, clients_lock):
    buffer = ""
    while True:
        try:
            data = client.recv(1024).decode()
            if not data:
                break
            buffer += data
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                line = line.strip()
                if not line:
                    continue
                parts = line.split()
                cmd = parts[0]
                if cmd == 'sync':
                    # Parse optional -t_1 value if present
                    t_1 = None
                    i = 1
                    while i < len(parts):
                        if parts[i] == '-t_1' and i + 1 < len(parts):
                            try:
                                t_1 = int(parts[i+1])
                            except ValueError:
                                pass
                            i += 2
                        else:
                            i += 1
                    t_2 = day_micro()
                    t_3 = day_micro()
                    resp = f"syncresp -t1 {t_1 if t_1 is not None else 0} -t2 {t_2} -t3 {t_3}\n"
                    try:
                        client.sendall(resp.encode())
                        print(f"SERVER: sync from {addr}, t_1={t_1}, t_2={t_2}, t_3={t_3}")
                    except Exception as e:
                        print(f"SERVER: failed sending syncresp to {addr}: {e}")
                else:
                    # Other commands ignored here
                    pass
        except Exception as e:
            print(f"SERVER: client {addr} error: {e}")
            break
    # Cleanup
    try:
        client.close()
    except Exception:
        pass
    with clients_lock:
        if client in clients:
            clients.remove(client)
    print(f"SERVER: client {addr} disconnected and removed")