import json
import time

def broadcast_all(command_line: str, clients, clients_lock):
    """Send command_line to every connected client."""
    with clients_lock:
        for c in list(clients):
            try:
                c.sendall(command_line.encode())
            except Exception as e:
                print(f"SERVER: failed to send to client: {e}")
                try:
                    c.close()
                except Exception:
                    pass
                clients.remove(c)

def broadcast_single(command_line: str, clients, clients_lock, target_ip: str):
    """Send command_line only to the client whose remote IP matches target_ip."""
    sent = False
    with clients_lock:
        for c in list(clients):
            try:
                peer_ip = c.getpeername()[0]
            except Exception:
                peer_ip = None
            if peer_ip == target_ip:
                try:
                    c.sendall(command_line.encode())
                    sent = True
                except Exception as e:
                    print(f"SERVER: failed to send to {target_ip}: {e}")
                    try:
                        c.close()
                    except Exception:
                        pass
                    clients.remove(c)
                break
    if not sent:
        print(f"SERVER: no active client with IP {target_ip}")

def send_play_with_sync(command_line, clients, clients_lock, delay: float = 0.05, target_ip = None):
    """Send a sync request then the play command. If target_ip provided, only to that IP."""
    sync_line = "sync\n"
    if target_ip:
        broadcast_single(sync_line, clients, clients_lock, target_ip)
        if delay > 0:
            time.sleep(delay)
        broadcast_single(command_line, clients, clients_lock, target_ip)
    else:
        broadcast_all(sync_line, clients, clients_lock)
        if delay > 0:
            time.sleep(delay)
        broadcast_all(command_line, clients, clients_lock)

# Optional backward compatibility alias
broadcast = broadcast_all
