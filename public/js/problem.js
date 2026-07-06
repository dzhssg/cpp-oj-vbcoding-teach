var difficultyConfig = {
  'Easy':   { cls: 'difficulty-easy',   label: 'Easy' },
  'Medium': { cls: 'difficulty-medium', label: 'Medium' },
  'Hard':   { cls: 'difficulty-hard',   label: 'Hard' }
};

var stateLoading = document.getElementById('state-loading');
var stateError   = document.getElementById('state-error');
var stateEmpty   = document.getElementById('state-empty');
var tableWrapper = document.getElementById('table-wrapper');
var tableBody    = document.getElementById('table-body');
var errMessage   = document.getElementById('error-message');
var btnRetry     = document.getElementById('btn-retry');

function hideAll() {
  stateLoading.classList.remove('visible');
  stateError.classList.remove('visible');
  stateEmpty.classList.remove('visible');
  tableWrapper.style.display = 'none';
}

function showLoading() {
  hideAll();
  stateLoading.classList.add('visible');
}

function showError(msg) {
  hideAll();
  errMessage.textContent = msg || '无法获取题目数据';
  stateError.classList.add('visible');
}

function showEmpty() {
  hideAll();
  stateEmpty.classList.add('visible');
}

function showTable(problems) {
  hideAll();
  if (!problems || problems.length === 0) {
    showEmpty();
    return;
  }

  tableBody.innerHTML = problems.map(function(p) {
    var diff = difficultyConfig[p.difficulty] || difficultyConfig['Easy'];
    return (
      '<tr data-id="' + p.id + '" class="clickable-row" onclick="navigateToProblem(' + p.id + ')">' +
        '<td class="col-id">' + p.id + '</td>' +
        '<td class="col-title">' + escapeHtml(p.title) + '</td>' +
        '<td><span class="difficulty-badge ' + diff.cls + '">' + diff.label + '</span></td>' +
      '</tr>'
    );
  }).join('');

  tableWrapper.style.display = 'block';
}

function navigateToProblem(id) {
  window.location.href = '/problem.html?id=' + id;
}

function escapeHtml(str) {
  var div = document.createElement('div');
  div.textContent = str;
  return div.innerHTML;
}

function delegateClick(parent, selector, handler) {
  parent.addEventListener('click', function(e) {
    var target = e.target.closest(selector);
    if (target && parent.contains(target)) {
      handler(target);
    }
  });
}

async function loadProblems() {
  showLoading();
  try {
    var data = await api.getProblems();
    showTable(data);
  } catch (err) {
    showError(err.message);
  }
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
    try {
      await api.logout();
    } catch (e) {}
    auth.logout();
    window.location.href = '/login.html';
  });
}

btnRetry.addEventListener('click', function() {
  loadProblems();
});

setupNavbar();

if (auth.requireAuth()) {
  loadProblems();
}
