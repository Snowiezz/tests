// backend/gradeAnswer.js
// Backend grading logic for A-Level CS responses.
// It reads the JSON mark scheme, grades each submitted answer,
// and returns per-question feedback plus a total score.
import { readFile } from 'node:fs/promises';

async function loadMarkScheme() {
  const fileUrl = new URL('./markScheme.json', import.meta.url);
  const raw = await readFile(fileUrl, 'utf-8');
  return JSON.parse(raw);
}

function normalizeText(value) {
  return String(value || '').trim().toLowerCase();
}

function gradeSingleAnswer(question, studentAnswer) {
  const answer = normalizeText(studentAnswer);

  if (!answer) {
    return {
      score: 0,
      feedback: 'No answer provided. Add an answer to earn marks.',
    };
  }

  if (question.type === 'mcq') {
    const correct = normalizeText(question.expectedAnswer);
    const isCorrect = answer === correct || answer === `${correct})`;

    return {
      score: isCorrect ? question.maxScore : 0,
      feedback: isCorrect
        ? 'Correct answer. Well done.'
        : `Incorrect. The expected answer is ${question.expectedAnswer}.`,
    };
  }

  const keywords = (question.keywords || []).map((keyword) => normalizeText(keyword));
  const matchedKeywords = keywords.filter((keyword) => answer.includes(keyword));
  const coverage = keywords.length > 0 ? matchedKeywords.length / keywords.length : 0;
  const score = Math.round(coverage * question.maxScore);

  const missingKeywords = keywords.filter((keyword) => !matchedKeywords.includes(keyword));

  return {
    score,
    feedback:
      missingKeywords.length === 0
        ? 'Strong answer. You covered all key points in the mark scheme.'
        : `Good attempt. To improve, mention: ${missingKeywords.join(', ')}.`,
  };
}

export async function gradeAnswers(submittedAnswers = []) {
  const markScheme = await loadMarkScheme();
  const results = {};
  let totalScore = 0;

  for (const question of markScheme.questions) {
    const submission = submittedAnswers.find((item) => item.id === question.id);
    const grade = gradeSingleAnswer(question, submission?.answer || '');

    results[question.id] = grade;
    totalScore += grade.score;
  }

  return {
    results,
    totalScore,
  };
}

export async function getPublicQuestions() {
  const markScheme = await loadMarkScheme();

  return markScheme.questions.map((question) => ({
    id: question.id,
    type: question.type,
    prompt: question.prompt,
    options: question.options || [],
    maxScore: question.maxScore,
  }));
}
