const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const path = require('path');

const app = express();
const server = http.createServer(app);

// Habilita CORS
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

app.use(express.static(path.join(__dirname, 'public')));

// Variável para controlar os slots de jogadores
let controllers = {
    player1: null, // Guarda o socket.id do Player 1
    player2: null  // Guarda o socket.id do Player 2
};

io.on('connection', (socket) => {
    // Captura o tipo de cliente enviado durante a conexão
    // Pode ser 'simulator' ou 'controller'
    const clientType = socket.handshake.query.type || 'controller';

    if (clientType === 'simulator') {
        // Se for o simulador, colocamos ele em uma "sala" (room) do socket.io
        // Isso facilita mandar os dados apenas para a tela do jogo depois
        socket.join('simulators');
        console.log(`Simulador conectado: ${socket.id}`);
    } else {
        // Lógica para Controles (Celular, ESP32)
        let role = 'Espectador';

        // Verifica se a vaga 1 está livre
        if (!controllers.player1) {
            controllers.player1 = socket.id;
            role = 'Player 1';
        } 
        // Se a 1 estiver ocupada, verifica a vaga 2
        else if (!controllers.player2) {
            controllers.player2 = socket.id;
            role = 'Player 2';
        }

        // Salva o papel no próprio socket para facilitar depois
        socket.role = role; 
        socket.emit('role', role);
        console.log(`Controle conectado: ${socket.id} -> ${role}`);
    }

    socket.on('move', (data) => {
        // Apenas repassa o movimento se quem enviou for P1 ou P2
        if (socket.role === 'Player 1' || socket.role === 'Player 2') {
            // Emite APENAS para os clientes que estão na sala 'simulators'
            io.to('simulators').emit('update_simulator', {
                player: socket.role,
                beta: data.beta
            });
        }
    });

    socket.on('disconnect', () => {
        if (clientType === 'simulator') {
            console.log(`Simulador desconectado: ${socket.id}`);
        } else {
            console.log(`Controle desconectado: ${socket.id} (${socket.role})`);
            // Libera a vaga caso um jogador caia
            if (controllers.player1 === socket.id) controllers.player1 = null;
            if (controllers.player2 === socket.id) controllers.player2 = null;
        }
    });
});

// Corrigido o console.log que dizia 3000 mas rodava na 3001
server.listen(3001, () => console.log('Servidor Node.js ativo na porta 3001'));
