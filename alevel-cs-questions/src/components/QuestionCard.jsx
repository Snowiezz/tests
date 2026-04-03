// src/components/QuestionCard.jsx
// Reusable single-question UI card.
// This card is used in a one-question-at-a-time quiz flow.
import React from 'react';

function QuestionCard({
  question,
  answer,
  onAnswerChange,
  result,
  currentIndex,
  totalQuestions,
}) {
  return (
    <article className="question-card">
      <p className="question-count">
        Question {currentIndex + 1} of {totalQuestions}
      </p>

      <h2>{question.prompt}</h2>

      {question.type === 'mcq' && (
        <div className="mcq-options">
          <p>Select an option and type your final answer in the box:</p>
          <ul>
            {question.options.map((option) => (
              <li key={option}>{option}</li>
            ))}
          </ul>
        </div>
      )}

      <label htmlFor={`answer-${question.id}`} className="answer-label">
        Your answer
      </label>
      <textarea
        id={`answer-${question.id}`}
        value={answer}
        onChange={(event) => onAnswerChange(question.id, event.target.value)}
        placeholder="Write your answer here..."
        rows={6}
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
