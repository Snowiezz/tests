// src/App.jsx
// Main page for the A-Level Computer Science practice site.
// It loads questions from the backend, tracks answers with React state,
// submits answers, and shows per-question feedback + total score.
import { useEffect, useMemo, useState } from 'react';
import QuestionCard from './components/QuestionCard';

const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || 'http://localhost:3001';

function App() {
  const [questions, setQuestions] = useState([]);
  const [answers, setAnswers] = useState({});
  const [results, setResults] = useState({});
  const [totalScore, setTotalScore] = useState(0);
  const [maxScore, setMaxScore] = useState(0);
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

  const handleAnswerChange = (id, value) => {
    setAnswers((previous) => ({
      ...previous,
      [id]: value,
    }));
  };

  const handleSubmit = async (event) => {
    event.preventDefault();
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
    <main className="app-shell">
      <header className="app-header">
        <h1>A-Level Computer Science Practice</h1>
        <p>Answer each question below, then submit to receive marks and feedback.</p>
      </header>

      {error && <p className="error-banner">{error}</p>}

      {loading ? (
        <p className="status">Loading questions...</p>
      ) : (
        <form onSubmit={handleSubmit} className="question-form">
          {questions.map((question) => (
            <QuestionCard
              key={question.id}
              question={question}
              answer={answers[question.id] || ''}
              onAnswerChange={handleAnswerChange}
              result={results[question.id]}
            />
          ))}

          <div className="submit-row">
            <p>
              Answered: <strong>{answeredCount}</strong> / {questions.length}
            </p>
            <button type="submit" disabled={submitting}>
              {submitting ? 'Checking...' : 'Submit Answers'}
            </button>
          </div>
        </form>
      )}

      {!loading && Object.keys(results).length > 0 && (
        <section className="score-panel">
          <h2>
            Total Score: {totalScore} / {maxScore}
          </h2>
          <p>Review each question card for detailed feedback.</p>
        </section>
      )}
    </main>
  );
}

export default App;
