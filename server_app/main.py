import socket
import threading
from parser_utils import parse_input
from handler import handle_client
from sender import broadcast_all, broadcast_single, send_play_with_sync

HOST = '0.0.0.0'
# PORT = 5000
PORT = 5001

clients = []
clients_lock = threading.Lock()


def accept_loop(server_sock):
    while True:
        try:
            print("SERVER: ready to connect")
            client, addr = server_sock.accept()
            with clients_lock:
                clients.append(client)
            print(f"SERVER: start client handler for {addr}")
            threading.Thread(target=handle_client, args=(
                client, addr, clients, clients_lock), daemon=True).start()
        except Exception as e:
            print(f"SERVER: accept error {e}")
            break


def main():
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((HOST, PORT))
    server_sock.listen(20)
    print(f"SERVER: listening on {HOST}:{PORT}")

    threading.Thread(target=accept_loop, args=(
        server_sock,), daemon=True).start()

    try:
        while True:
            user_input = input("SERVER: Enter:")
            if user_input.strip().lower() in ["exit", "quit"]:
                print("SERVER: quit")
                break
            # Optional syntax: @IP command ... for single target
            target_ip = None
            if user_input.startswith('@'):
                parts = user_input.split(maxsplit=1)
                if len(parts) == 2:
                    target_ip = parts[0][1:]
                    user_input = parts[1]
                else:
                    print("SERVER: invalid targeted command format")
                    continue
            command = parse_input(user_input)
            if not command:
                continue
            line = command + '\n'
            if command.startswith('play'):
                send_play_with_sync(
                    line, clients, clients_lock, target_ip=target_ip)
            else:
                if target_ip:
                    broadcast_single(line, clients, clients_lock, target_ip)
                else:
                    broadcast_all(line, clients, clients_lock)
    except KeyboardInterrupt:
        print("SERVER: interrupted")
    finally:
        with clients_lock:
            for c in clients:
                try:
                    c.close()
                except Exception:
                    pass
            clients.clear()
        server_sock.close()
        print("SERVER: shutdown complete")


if __name__ == '__main__':
    main()
