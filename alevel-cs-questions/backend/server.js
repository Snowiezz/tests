import http from 'node:http';
import { gradeAnswers, getPublicQuestions } from './gradeAnswer.js';

const PORT = 3001;

function sendJson(response, statusCode, payload) {
  response.writeHead(statusCode, {
    'Content-Type': 'application/json',
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET,POST,OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
  });
  response.end(JSON.stringify(payload));
}

const server = http.createServer(async (request, response) => {
  if (request.method === 'OPTIONS') {
    sendJson(response, 200, { ok: true });
    return;
  }

  if (request.method === 'GET' && request.url === '/api/questions') {
    try {
      const questions = await getPublicQuestions();
      sendJson(response, 200, { questions });
    } catch {
      sendJson(response, 500, { error: 'Failed to load questions.' });
    }
    return;
  }

  if (request.method === 'POST' && request.url === '/api/grade') {
    let body = '';

    request.on('data', (chunk) => {
      body += chunk;
    });

    request.on('end', async () => {
      try {
        const parsed = JSON.parse(body || '{}');
        const grading = await gradeAnswers(parsed.answers || []);
        sendJson(response, 200, grading);
      } catch {
        sendJson(response, 400, { error: 'Invalid grading payload.' });
      }
    });
    return;
  }

  sendJson(response, 404, { error: 'Route not found.' });
});

server.listen(PORT, () => {
  console.log(`Backend grading server running on http://localhost:${PORT}`);
});
