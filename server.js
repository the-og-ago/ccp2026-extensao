const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const path = require('path');

const app = express();
const server = http.createServer(app);

// Habilita CORS para aceitar conexões vindas do Cloudflare Tunnel
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

app.use(express.static(path.join(__dirname, 'public')));
let players = {};

io.on('connection', (socket) => {
    const numPlayers = Object.keys(players).length;
    players[socket.id] = numPlayers === 0 ? 'Player 1' : (numPlayers === 1 ? 'Player 2' : 'Espectador');
    socket.emit('role', players[socket.id]);

    socket.on('move', (data) => {
        // Envia os dados para o simulador
        io.emit('update_simulator', {
            player: players[socket.id],
            beta: data.beta
        });
    });

    socket.on('disconnect', () => delete players[socket.id]);
});

server.listen(3001, () => console.log('Servidor Node.js ativo na porta 3000'));
