import tkinter as tk
from tkinter import messagebox
import socket
import threading

class TicTacToe(tk.Tk):
    def __init__(self, host='127.0.0.1', port=5000):
        super().__init__()
        self.title("Tic Tac Toe")
        self.geometry("300x300")
        self.resizable(False, False)
        self.player = None
        self.board = [['' for _ in range(3)] for _ in range(3)]
        self.create_widgets()

        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((host, port))
        threading.Thread(target=self.receive_data).start()

    def create_widgets(self):
        for i in range(3):
            for j in range(3):
                button = tk.Button(self, text="", width=10, height=3, command=lambda i=i, j=j: self.on_button_click(i, j))
                button.grid(row=i, column=j)
        reset_button = tk.Button(self, text="Reset", width=10, height=1, command=self.reset_game)
        reset_button.grid(row=3, column=1)
    def on_button_click(self, i, j):
        if self.board[i][j] == '' and self.player:
            self.board[i][j] = self.player
            button = self.grid_slaves(row=i, column=j)[0]
            button.config(text=self.player)
            self.client_socket.sendall(f"{i},{j},{self.player}".encode('utf-8'))

    def receive_data(self):
        while True:
            try:
                msg = self.client_socket.recv(1024).decode('utf-8')
                data = msg.split(',')
                if len(data) == 1:
                    if data[0] == "X" or data[0] == "O":
                        self.player = data[0]
                        self.title(f"Tic Tac Toe - Player {self.player}")
                elif msg == "RESET":
                    self.reset_game()
                elif len(data) == 3:
                    i, j, other_player = data
                    self.board[int(i)][int(j)] = other_player
                    button = self.grid_slaves(row=int(i), column=int(j))[0]
                    button.config(text=other_player)
                    if self.check_winner():
                        messagebox.showinfo("Tic Tac Toe", f"Player {other_player} wins!")
                        self.reset_game()
                    elif self.check_draw():
                        messagebox.showinfo("Tic Tac Toe", "It's a draw!")
                        self.reset_game()
            except Exception as e:
                print(f"Error: {e}")
                self.client_socket.close()
                break

    def check_winner(self):
        for row in self.board:
            if row[0] == row[1] == row[2] != '':
                return True
        for col in range(3):
            if self.board[0][col] == self.board[1][col] == self.board[2][col] != '':
                return True
        if self.board[0][0] == self.board[1][1] == self.board[2][2] != '':
            return True
        if self.board[0][2] == self.board[1][1] == self.board[2][0] != '':
            return True
        return False

    def check_draw(self):
        for row in self.board:
            for cell in row:
                if cell == '':
                    return False
        return True

    def reset_game(self):
        if self.player is not None:
            self.board = [['' for _ in range(3)] for _ in range(3)]
            for i in range(3):
                for j in range(3):
                    button = self.grid_slaves(row=i, column=j)[0]
                    button.config(text="")
            self.player = None
            self.client_socket.sendall("RESET".encode('utf-8'))

if __name__ == "__main__":
    app = TicTacToe()
    app.mainloop()
