// src/components/QuestionCard.jsx
// Reusable question UI card.
// It supports both MCQ (display options) and open-ended prompts,
// and always provides a textarea for user responses.
import React from 'react';

function QuestionCard({ question, answer, onAnswerChange, result }) {
  return (
    <article className="question-card">
      <h2>
        Q{question.id}. {question.prompt}
      </h2>

      {question.type === 'mcq' && (
        <div className="mcq-options">
          <p>Options:</p>
          <ul>
            {question.options.map((option) => (
              <li key={option}>{option}</li>
            ))}
          </ul>
          <p className="hint">Enter the letter (e.g., A).</p>
        </div>
      )}

      <label htmlFor={`answer-${question.id}`} className="answer-label">
        Your answer
      </label>
      <textarea
        id={`answer-${question.id}`}
        value={answer}
        onChange={(event) => onAnswerChange(question.id, event.target.value)}
        placeholder="Type your answer here..."
        rows={5}
      />

      {result && (
        <div className="feedback-box">
          <p>
            <strong>Score:</strong> {result.score} / {question.maxScore}
          </p>
          <p>
            <strong>Feedback:</strong> {result.feedback}
          </p>
        </div>
      )}
    </article>
  );
}

export default QuestionCard;
