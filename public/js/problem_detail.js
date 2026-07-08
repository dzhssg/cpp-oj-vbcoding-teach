var problemId = null;
var editor  = null;
var templateCode = '';

var elLoading    = document.getElementById('problem-loading');
var elError      = document.getElementById('problem-error');
var elErrorText  = document.getElementById('problem-error-text');
var elContent    = document.getElementById('problem-content');
var elTitle      = document.getElementById('problem-title');
var elIdLabel    = document.getElementById('problem-id-label');
var elDifficulty = document.getElementById('problem-difficulty');
var elBody       = document.getElementById('problem-body');

var btnSubmit    = document.getElementById('btn-submit');
var btnResubmit  = document.getElementById('btn-resubmit');
var resultsPanel = document.getElementById('results-panel');
var resultBadge  = document.getElementById('result-badge');
var resultProgress = document.getElementById('result-progress');
var resultsBody  = document.getElementById('results-body');

function getProblemId() {
  var params = new URLSearchParams(window.location.search);
  var id = parseInt(params.get('id'), 10);
  if (!id || id < 1) {
    showError('无效的题目 ID，请从题目列表进入。');
    return null;
  }
  return id;
}

function showLoading() {
  elLoading.classList.add('visible');
  elError.classList.remove('visible');
  elContent.style.display = 'none';
}

function showError(msg) {
  elLoading.classList.remove('visible');
  elError.classList.add('visible');
  elContent.style.display = 'none';
  elErrorText.textContent = msg || '加载失败';
}

function showContent(problem) {
  elLoading.classList.remove('visible');
  elError.classList.remove('visible');
  elContent.style.display = 'block';

  elTitle.textContent = problem.title;
  elIdLabel.textContent = '#' + problem.id;

  var diff = difficultyConfig[problem.difficulty] || difficultyConfig['Easy'];
  elDifficulty.innerHTML = '<span class="difficulty-badge ' + diff.cls + '">' + diff.label + '</span>';

  elBody.innerHTML = renderMarkdown(problem.content || '');

  if (problem.template) {
    templateCode = problem.template;
  }

  renderSolution(problem);
}

function renderSolution(problem) {
  var hasSolutionContent = problem.solution_content && problem.solution_content.trim();
  var hasSolutionCode = problem.solution_code && problem.solution_code.trim();

  if (!hasSolutionContent && !hasSolutionCode) {
    document.getElementById('solution-section').style.display = 'none';
    return;
  }

  document.getElementById('solution-section').style.display = 'block';

  if (hasSolutionContent) {
    document.getElementById('solution-explanation').style.display = 'block';
    document.getElementById('solution-explanation').innerHTML = renderMarkdown(problem.solution_content);
  } else {
    document.getElementById('solution-explanation').style.display = 'none';
  }

  if (hasSolutionCode) {
    document.getElementById('solution-code-section').style.display = 'block';
    document.getElementById('solution-code-display').textContent = problem.solution_code;
  } else {
    document.getElementById('solution-code-section').style.display = 'none';
  }
}

var difficultyConfig = {
  'Easy':   { cls: 'difficulty-easy',   label: 'Easy' },
  'Medium': { cls: 'difficulty-medium', label: 'Medium' },
  'Hard':   { cls: 'difficulty-hard',   label: 'Hard' }
};

function renderMarkdown(md) {
  var lines = md.split('\n');
  var html = [];
  var inCodeBlock = false;
  var codeContent = [];
  var lang = '';

  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];

    if (inCodeBlock) {
      if (/^```/.test(line.trim())) {
        html.push('<pre><code>' + escapeHtml(codeContent.join('\n')) + '</code></pre>');
        codeContent = [];
        inCodeBlock = false;
      } else {
        codeContent.push(line);
      }
      continue;
    }

    if (/^```(\w*)$/.test(line.trim())) {
      inCodeBlock = true;
      continue;
    }

    if (/^###\s/.test(line)) {
      html.push('<h3>' + escapeHtml(line.replace(/^###\s*/, '')) + '</h3>');
    } else if (/^##\s/.test(line)) {
      html.push('<h2>' + escapeHtml(line.replace(/^##\s*/, '')) + '</h2>');
    } else if (/^#\s/.test(line)) {
      html.push('<h2>' + escapeHtml(line.replace(/^#\s*/, '')) + '</h2>');
    } else if (/^\s*[-*+]\s/.test(line)) {
      html.push('<li>' + processInline(line.replace(/^\s*[-*+]\s*/, '')) + '</li>');
    } else if (/^\s*\d+\.\s/.test(line)) {
      html.push('<li>' + processInline(line.replace(/^\s*\d+\.\s*/, '')) + '</li>');
    } else if (line.trim() === '') {
      html.push('<br>');
    } else {
      html.push('<p>' + processInline(line) + '</p>');
    }
  }

  if (inCodeBlock && codeContent.length > 0) {
    html.push('<pre><code>' + escapeHtml(codeContent.join('\n')) + '</code></pre>');
  }

  return html.join('\n');
}

function processInline(text) {
  text = escapeHtml(text);
  text = text.replace(/`([^`]+)`/g, '<code>$1</code>');
  text = text.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
  text = text.replace(/__([^_]+)__/g, '<strong>$1</strong>');
  text = text.replace(/\*([^*]+)\*/g, '<em>$1</em>');
  text = text.replace(/_([^_]+)_/g, '<em>$1</em>');
  return text;
}

function escapeHtml(str) {
  var div = document.createElement('div');
  div.textContent = str;
  return div.innerHTML;
}

function isMobileViewport() {
  return window.matchMedia && window.matchMedia('(max-width: 900px)').matches;
}

function ensureEditorHeight() {
  var container = document.querySelector('.editor-container');
  var editorEl = document.getElementById('editor');
  if (isMobileViewport()) {
    var h = Math.max(360, Math.round(window.innerHeight * 0.6)) + 'px';
    if (container) {
      container.style.flex = 'none';
      container.style.height = h;
      container.style.minHeight = '360px';
    }
    if (editorEl) {
      editorEl.style.height = h;
      editorEl.style.minHeight = '360px';
    }
  }
}

function initEditorFallback() {
  var fallback = document.createElement('textarea');
  fallback.id = 'editor-fallback';
  fallback.style.cssText = 'width:100%;height:100%;min-height:360px;box-sizing:border-box;background:#0B1121;color:#F8FAFC;border:none;padding:16px;font-family:monospace;font-size:14px;resize:none;outline:none;';
  fallback.value = templateCode || '#include <iostream>\n\nint main() {\n  \n  return 0;\n}\n';
  var editorEl = document.getElementById('editor');
  if (editorEl) editorEl.style.display = 'none';
  document.querySelector('.editor-container').appendChild(fallback);
  editor = fallback;
}

function initEditor(retries) {
  ensureEditorHeight();

  if (typeof ace === 'undefined') {
    retries = (retries == null) ? 0 : retries;
    if (retries < 40) {
      setTimeout(function() { initEditor(retries + 1); }, 150);
      return;
    }
    console.warn('Ace.js not loaded after waiting, using textarea fallback');
    initEditorFallback();
    return;
  }

  editor = ace.edit('editor');
  editor.setTheme('ace/theme/monokai');
  editor.session.setMode('ace/mode/c_cpp');
  editor.setOptions({
    fontSize: '14px',
    fontFamily: "'JetBrains Mono', 'Fira Code', 'Courier New', monospace",
    showLineNumbers: true,
    showPrintMargin: false,
    tabSize: 4,
    useSoftTabs: false,
    highlightActiveLine: true,
    enableBasicAutocompletion: true,
    enableLiveAutocompletion: true,
    enableSnippets: true
  });

  editor.setValue(templateCode || '#include <iostream>\n\nint main() {\n  \n  return 0;\n}\n', -1);
  editor.clearSelection();

  var doResize = function() {
    ensureEditorHeight();
    editor.resize(true);
  };
  setTimeout(doResize, 0);
  setTimeout(doResize, 300);
  setTimeout(doResize, 800);
  window.addEventListener('resize', doResize);
  window.addEventListener('orientationchange', function() { setTimeout(doResize, 200); });
  if (window.ResizeObserver) {
    var ro = new ResizeObserver(function() { editor.resize(true); });
    ro.observe(document.querySelector('.editor-container'));
  }
}

function setSubmitLoading(btn, state) {
  btn.classList.toggle('loading', state);
  btn.disabled = state;
}

function scrollToResults() {
  resultsPanel.scrollIntoView({ behavior: 'smooth', block: 'start' });
}

function showResults(data) {
  resultsPanel.classList.add('visible');
  var status = data.status;

  resultBadge.textContent = status;
  resultBadge.className = 'results-status-badge ' + status.toLowerCase();

  if (status === 'CE') {
    resultProgress.textContent = '编译错误';
    renderCompileError(data.compile_error);
  } else {
    var passed = data.passed || 0;
    var total  = data.total || 0;
    resultProgress.innerHTML = '通过 <strong>' + passed + '</strong> / ' + total + ' 个测试用例';
    renderTestCases(data.results || []);
  }

  scrollToResults();
}

function renderCompileError(error) {
  resultsBody.innerHTML =
    '<div class="results-ce-label">编译错误信息</div>' +
    '<div class="results-ce">' + escapeHtml(error || 'Unknown compile error') + '</div>';
}

function renderTestCases(results) {
  if (results.length === 0) {
    resultsBody.innerHTML =
      '<div class="results-ce" style="color: var(--color-text-muted);">无测试用例结果</div>';
    return;
  }

  var html = ['<div class="test-case-results">'];
  for (var i = 0; i < results.length; i++) {
    var tc = results[i];
    var statusClass = tc.status.toLowerCase();
    var iconSvg = getStatusIcon(tc.status);

    html.push(
      '<div class="test-case-item">' +
        '<div class="test-case-item-header" onclick="toggleTestCase(this.parentElement)">' +
          '<span class="test-case-index">Case #' + (tc.position + 1) + '</span>' +
          '<span class="test-case-status-icon ' + statusClass + '">' + iconSvg + '</span>' +
          '<span style="font-size:0.8rem;color:var(--color-text-muted);">' + tc.status + '</span>' +
          '<svg class="test-case-arrow" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">' +
            '<polyline points="6 9 12 15 18 9"/>' +
          '</svg>' +
        '</div>' +
        '<div class="test-case-item-detail">' +
          '<div class="test-case-detail-row">' +
            '<div class="test-case-detail-label">输入</div>' +
            '<div class="test-case-detail-value">' + (tc.input ? escapeHtml(tc.input) : '(空)') + '</div>' +
          '</div>' +
          '<div class="test-case-detail-row">' +
            '<div class="test-case-detail-label">期望输出</div>' +
            '<div class="test-case-detail-value">' + (tc.expected ? escapeHtml(tc.expected) : '(空)') + '</div>' +
          '</div>' +
          '<div class="test-case-detail-row">' +
            '<div class="test-case-detail-label">实际输出</div>' +
            '<div class="test-case-detail-value">' + (tc.actual !== undefined ? escapeHtml(tc.actual) : '(无)') + '</div>' +
          '</div>' +
          (tc.error ? (
            '<div class="test-case-detail-row">' +
              '<div class="test-case-detail-label" style="color:var(--color-error);">错误信息</div>' +
              '<div class="test-case-detail-value" style="color:#FCA5A5;">' + escapeHtml(tc.error) + '</div>' +
            '</div>'
          ) : '') +
        '</div>' +
      '</div>'
    );
  }
  html.push('</div>');

  resultsBody.innerHTML = html.join('');
}

function getStatusIcon(status) {
  switch (status) {
    case 'AC':
      return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><polyline points="8 12 11 15 16 9"/></svg>';
    case 'WA':
      return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>';
    case 'TLE':
      return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>';
    case 'RE':
      return '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>';
    default:
      return '';
  }
}

function toggleTestCase(el) {
  el.classList.toggle('expanded');
}

function setupNavbar() {
  var navUsername = document.getElementById('nav-username');
  var navRole     = document.getElementById('nav-role');
  var btnLogout   = document.getElementById('btn-logout');

  var user = auth.getUser();
  if (user) {
    navUsername.textContent = user.username;
    if (user.role === 'admin') {
      navRole.style.display = 'inline-block';
      var adminLink = document.getElementById('nav-admin-link');
      if (adminLink) adminLink.style.display = 'flex';
    }
  }

  btnLogout.addEventListener('click', async function() {
    try { await api.logout(); } catch (e) {}
    auth.logout();
    window.location.href = '/login.html';
  });
}

async function handleSubmit(btn) {
  if (!problemId) return;

  var code;
  if (editor && typeof editor.getValue === 'function') {
    code = editor.getValue();
  } else if (editor && editor.value !== undefined) {
    code = editor.value;
  } else {
    code = '';
  }

  if (!code || !code.trim()) {
    alert('请编写代码后再提交');
    return;
  }

  setSubmitLoading(btn, true);

  try {
    var data = await api.submit(problemId, code);
    showResults(data);
    btnSubmit.style.display = 'none';
  } catch (err) {
    alert('提交失败: ' + (err.message || '网络错误'));
  } finally {
    setSubmitLoading(btn, false);
  }
}

function setupSolutionToggle() {
  var toggle = document.getElementById('solution-toggle');
  if (!toggle) return;
  toggle.addEventListener('click', function() {
    document.getElementById('solution-section').classList.toggle('collapsed');
  });
}

async function main() {
  setupNavbar();

  if (!auth.requireAuth()) return;

  problemId = getProblemId();
  if (!problemId) return;

  showLoading();

  try {
    var problem = await api.getProblem(problemId);
    showContent(problem);
  } catch (err) {
    showError(err.message || '无法加载题目');
    return;
  }

  initEditor();

  setupSolutionToggle();

  btnSubmit.addEventListener('click', function() { handleSubmit(btnSubmit); });
  btnResubmit.addEventListener('click', function() {
    btnSubmit.style.display = 'inline-flex';
    resultsPanel.classList.remove('visible');
    handleSubmit(btnSubmit);
  });
}

document.addEventListener('DOMContentLoaded', main);
