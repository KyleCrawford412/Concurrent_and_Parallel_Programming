class TicTacToeServer:
    def __init__(self, host='0.0.0.0', port=5000):
        self.host = host
        self.port = port
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.bind((self.host, self.port))
        self.server.listen(2)
        self.clients = []

    def handle_client(self, client):
        player_symbol = 'X' if len(self.clients) == 1 else 'O'
        client.sendall(player_symbol.encode('utf-8'))
        while True:
            try:
                msg = client.recv(1024).decode('utf-8')
                if msg:
                    for other_client in self.clients:
                        if other_client != client:
                            other_client.sendall(msg.encode('utf-8'))
                    if msg == "RESET":
                        for idx, player_client in enumerate(self.clients):
                            
                            player_client.sendall(("X" if idx == 0 else "O").encode('utf-8'))
            except Exception as e:
                print(f"Error: {e}")
                client.close()
                self.clients.remove(client)
                break

    def start(self):
        print("Tic Tac Toe server is running...")
        while True:
            client, _ = self.server.accept()
            self.clients.append(client)
            print(f"Connected to a client")
            threading.Thread(target=self.handle_client, args=(client,)).start()


if __name__ == "__main__":
    server = TicTacToeServer()
    server.start()
