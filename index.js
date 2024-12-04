/*  

    1- get the audio from the audio.cpp file and pipe it to the client
    2- which can be used be used to send the audio to deepgram for 
    3- speech to text conversion

*/

const ws = require('ws');
const { spawn } = require('child_process');

let PORT = 0;

const wss = new ws.Server({ host: '127.0.0.1', port: PORT });

console.log(`Server started on port ${PORT}`);

wss.on('connection', (ws) => {
    console.log('Client connected');

    const audio = spawn('node', ['audio.js']);
    audio.stdout.on('data', (data) => {
        ws.send(data);
    });

    audio.stderr.on('data', (data) => {
        console.error(`stderr: ${data}`);
        ws.onerror(data);
    });

    ws.on('message', (message) => {
        console.log(`Received message => ${message}`);
    });

    ws.on('close', () => {
        console.log('Client disconnected');
        audio.kill();
    });
})