const http = require('http');
const fs = require('fs');
const path = require('path');

const DOCS_DIR = path.join(__dirname, 'docs');

const mimeTypes = {
    '.html': 'text/html; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.js': 'text/javascript; charset=utf-8',
    '.json': 'application/json; charset=utf-8',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.svg': 'image/svg+xml',
    '.webp': 'image/webp',
    '.ico': 'image/x-icon'
};

function requestHandler(req, res) {
    let reqPath = req.url.split('?')[0].split('#')[0];
    if (reqPath === '/' || reqPath === '') reqPath = '/index.html';
    const filePath = path.join(DOCS_DIR, reqPath);

    fs.readFile(filePath, (err, data) => {
        if (err) {
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            res.end('404 Not Found');
            return;
        }
        const ext = path.extname(filePath).toLowerCase();
        res.writeHead(200, {
            'Content-Type': mimeTypes[ext] || 'application/octet-stream',
            'Access-Control-Allow-Origin': '*',
            'Cache-Control': 'no-cache'
        });
        res.end(data);
    });
}

function startServer(port) {
    const server = http.createServer(requestHandler);
    server.on('error', (err) => {
        if (err.code === 'EADDRINUSE') {
            console.warn(`Port ${port} in use, skipping...`);
        } else {
            console.error(`Server error on port ${port}:`, err.message);
        }
    });
    server.listen(port, '0.0.0.0', () => {
        console.log(`Server listening on http://localhost:${port} & http://127.0.0.1:${port}`);
    });
    return server;
}

startServer(8080);
startServer(3000);
