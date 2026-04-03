// src/index.js
// Entry point: mounts the React app into the root element in index.html.
// Beginner note: Keep this file simple; app logic belongs in App.js.
import React from 'react';
import { createRoot } from 'react-dom/client';
import './index.css';
import App from './App';

const root = createRoot(document.getElementById('root'));
root.render(
  React.createElement(
    React.StrictMode,
    null,
    React.createElement(App)
  )
);
