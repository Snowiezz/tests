# A-Level Computer Science Questions Website

A beginner-friendly React + Node.js project for practicing A-Level Computer Science answers.

## What this app does

- Shows multiple-choice and open-ended questions.
- Gives each question a textarea for student answers.
- Sends answers to a separate backend grading endpoint.
- Grades answers against a sample JSON mark scheme.
- Returns per-question feedback and a total score.

## Project structure

```text
alevel-cs-questions/
├── backend/
│   ├── gradeAnswer.js      # Grading logic (uses mark scheme JSON)
│   ├── markScheme.json     # Sample mark scheme data
│   └── server.js           # Node HTTP API server
├── src/
│   ├── components/
│   │   └── QuestionCard.js # Reusable question + answer UI card
│   ├── App.js              # Main app state + submit workflow
│   ├── index.css           # Responsive styling
│   └── index.js            # React entry point
├── index.html
├── package.json
└── vite.config.js
```

## Run locally

1. Install dependencies:

```bash
npm install
```

2. Start backend (terminal 1):

```bash
npm run server
```

3. Start frontend (terminal 2):

```bash
npm run dev
```

4. Open the local URL shown by Vite (usually http://localhost:5173).

## Beginner notes

- Edit `backend/markScheme.json` to add/change questions and mark criteria.
- `App.js` handles loading questions, answer state, submission, and score display.
- `QuestionCard.js` is reusable, so each question uses the same UI component.
