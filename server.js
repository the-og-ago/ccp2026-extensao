const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// Serve os arquivos estáticos (nossa página HTML)
app.use(express.static(path.join(__dirname, 'public')));

let players = {};

io.on('connection', (socket) => {
    console.log(`Novo dispositivo conectado: ${socket.id}`);

    // Atribui o jogador como Player 1 ou Player 2
    const numPlayers = Object.keys(players).length;
    if (numPlayers === 0) {
        players[socket.id] = 'Player 1';
    } else if (numPlayers === 1) {
        players[socket.id] = 'Player 2';
    } else {
        players[socket.id] = 'Espectador';
    }

    console.log(`${socket.id} entrou como ${players[socket.id]}`);
    socket.emit('role', players[socket.id]); // Avisa o celular de qual player ele é

    // Recebe os dados do acelerômetro/giroscópio
    socket.on('move', (data) => {
        console.log(`Movimento do ${players[socket.id]}: Beta (Frente/Trás): ${data.beta.toFixed(2)}, Gamma (Esq/Dir): ${data.gamma.toFixed(2)}`);
        
        // No futuro, aqui você fará o broadcast(envio) desses dados para o ESP32
        // io.emit('esp32_update', { player: players[socket.id], ...data });
    });

    socket.on('disconnect', () => {
        console.log(`${players[socket.id]} desconectou.`);
        delete players[socket.id];
    });
});

const PORT = 3001;
server.listen(PORT, () => {
    console.log(`Servidor rodando em http://localhost:${PORT}`);
    console.log('Use o Cloudflare Tunnel para expor a porta 3000');
});
