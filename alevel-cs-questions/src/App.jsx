// src/App.jsx
// Main page for the A-Level Computer Science practice site.
// It loads questions from the backend, tracks answers with React state,
// presents one question at a time, submits answers, and shows score feedback.
import { useEffect, useMemo, useState } from 'react';
import QuestionCard from './components/QuestionCard';

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://localhost:3001';

function App() {
  const [questions, setQuestions] = useState([]);
  const [answers, setAnswers] = useState({});
  const [results, setResults] = useState({});
  const [totalScore, setTotalScore] = useState(0);
  const [maxScore, setMaxScore] = useState(0);
  const [currentIndex, setCurrentIndex] = useState(0);
  const [loading, setLoading] = useState(true);
  const [submitting, setSubmitting] = useState(false);
  const [error, setError] = useState('');

  useEffect(() => {
    async function loadQuestions() {
      setLoading(true);
      setError('');

      try {
        const response = await fetch(`${API_BASE_URL}/api/questions`);
        if (!response.ok) {
          throw new Error('Unable to load questions from the backend.');
        }

        const data = await response.json();
        setQuestions(data.questions);

        const defaultAnswers = data.questions.reduce((acc, question) => {
          acc[question.id] = '';
          return acc;
        }, {});
        setAnswers(defaultAnswers);

        const max = data.questions.reduce((sum, question) => sum + question.maxScore, 0);
        setMaxScore(max);
      } catch (err) {
        setError(err.message || 'Something went wrong while loading questions.');
      } finally {
        setLoading(false);
      }
    }

    loadQuestions();
  }, []);

  const answeredCount = useMemo(
    () => Object.values(answers).filter((value) => value.trim().length > 0).length,
    [answers]
  );

  const currentQuestion = questions[currentIndex];
  const isFirstQuestion = currentIndex === 0;
  const isLastQuestion = currentIndex === questions.length - 1;
  const hasResults = Object.keys(results).length > 0;

  const handleAnswerChange = (id, value) => {
    setAnswers((previous) => ({
      ...previous,
      [id]: value,
    }));
  };

  const handlePrevious = () => {
    setCurrentIndex((prev) => Math.max(0, prev - 1));
  };

  const handleNext = () => {
    setCurrentIndex((prev) => Math.min(questions.length - 1, prev + 1));
  };

  const handleSubmit = async () => {
    setSubmitting(true);
    setError('');

    try {
      const payload = {
        answers: questions.map((question) => ({
          id: question.id,
          answer: answers[question.id] || '',
        })),
      };

      const response = await fetch(`${API_BASE_URL}/api/grade`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(payload),
      });

      if (!response.ok) {
        throw new Error('Unable to grade answers right now.');
      }

      const data = await response.json();
      setResults(data.results);
      setTotalScore(data.totalScore);
    } catch (err) {
      setError(err.message || 'Something went wrong while grading.');
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="page-bg">
      <header className="top-nav">
        <div className="brand">CSQuiz Pro</div>
        <nav>
          <a href="#">Features</a>
          <a href="#">Practice</a>
          <a href="#">Pricing</a>
          <button type="button">Start Free</button>
        </nav>
      </header>

      <main className="app-shell">
        <p className="hero-copy">Take the quiz to test your understanding and track your grade.</p>

        {error && <p className="error-banner">{error}</p>}

        {loading ? (
          <p className="status">Loading questions...</p>
        ) : (
          <section className="quiz-panel">
            <div className="stepper" aria-label="Question progress">
              {questions.map((question, index) => {
                const isActive = index === currentIndex;
                const isAnswered = (answers[question.id] || '').trim().length > 0;

                return (
                  <button
                    key={question.id}
                    type="button"
                    className={`step-dot ${isActive ? 'active' : ''} ${isAnswered ? 'done' : ''}`}
                    onClick={() => setCurrentIndex(index)}
                    aria-label={`Go to question ${index + 1}`}
                  >
                    {index + 1}
                  </button>
                );
              })}
            </div>

            {currentQuestion && (
              <QuestionCard
                question={currentQuestion}
                answer={answers[currentQuestion.id] || ''}
                onAnswerChange={handleAnswerChange}
                result={results[currentQuestion.id]}
                currentIndex={currentIndex}
                totalQuestions={questions.length}
              />
            )}

            <div className="action-row">
              <button type="button" className="ghost-btn" onClick={handlePrevious} disabled={isFirstQuestion}>
                Previous
              </button>

              {!isLastQuestion ? (
                <button type="button" className="primary-btn" onClick={handleNext}>
                  Next
                </button>
              ) : (
                <button type="button" className="primary-btn" onClick={handleSubmit} disabled={submitting}>
                  {submitting ? 'Checking...' : 'Submit Quiz'}
                </button>
              )}
            </div>

            <div className="bottom-meta">
              <p>
                Answered <strong>{answeredCount}</strong> of {questions.length}
              </p>
            </div>
          </section>
        )}

        {!loading && hasResults && (
          <section className="score-panel">
            <h2>
              Final Score: {totalScore} / {maxScore}
            </h2>
            <p>Click any number above to review feedback for each question.</p>
          </section>
        )}
      </main>
    </div>
  );
}

export default App;
