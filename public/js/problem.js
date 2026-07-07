var difficultyConfig = {
  'Easy':   { cls: 'difficulty-easy',   label: 'Easy' },
  'Medium': { cls: 'difficulty-medium', label: 'Medium' },
  'Hard':   { cls: 'difficulty-hard',   label: 'Hard' }
};

var allProblems = [];
var currentFilter = 'All';

var stateLoading = document.getElementById('state-loading');
var stateError   = document.getElementById('state-error');
var stateEmpty   = document.getElementById('state-empty');
var tableWrapper = document.getElementById('table-wrapper');
var tableBody    = document.getElementById('table-body');
var errMessage   = document.getElementById('error-message');
var btnRetry     = document.getElementById('btn-retry');
var filterBar    = document.getElementById('filter-bar');
var filterCount  = document.getElementById('filter-count');

function hideAll() {
  stateLoading.classList.remove('visible');
  stateError.classList.remove('visible');
  stateEmpty.classList.remove('visible');
  tableWrapper.style.display = 'none';
}

function showLoading() {
  hideAll();
  filterBar.style.display = 'none';
  stateLoading.classList.add('visible');
}

function showError(msg) {
  hideAll();
  filterBar.style.display = 'none';
  errMessage.textContent = msg || '无法获取题目数据';
  stateError.classList.add('visible');
}

function showEmpty() {
  hideAll();
  filterBar.style.display = 'none';
  stateEmpty.classList.add('visible');
}

function renderTable() {
  var problems = allProblems;
  if (currentFilter !== 'All') {
    problems = allProblems.filter(function(p) {
      return p.difficulty === currentFilter;
    });
  }

  hideAll();

  filterBar.style.display = 'flex';
  filterCount.innerHTML = '共 <strong>' + problems.length + '</strong> 道题目';

  if (!problems || problems.length === 0) {
    var isEmpty = (currentFilter !== 'All')
      ? '当前筛选条件下暂无题目'
      : '题目列表为空，请联系管理员添加题目';

    stateEmpty.querySelector('p:last-child').textContent = isEmpty;
    showEmpty();
    filterBar.style.display = 'flex';
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

function setupFilters() {
  var pills = filterBar.querySelectorAll('.filter-pill');
  pills.forEach(function(pill) {
    pill.addEventListener('click', function() {
      pills.forEach(function(p) { p.classList.remove('active'); });
      pill.classList.add('active');
      currentFilter = pill.getAttribute('data-filter');
      renderTable();
    });
  });
}

async function loadProblems() {
  showLoading();
  try {
    var data = await api.getProblems();
    allProblems = data || [];
    currentFilter = 'All';
    renderTable();
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

setupFilters();
setupNavbar();

if (auth.requireAuth()) {
  loadProblems();
}
