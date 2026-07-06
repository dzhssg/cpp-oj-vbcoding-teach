var difficultyConfig = {
  'Easy':   { cls: 'difficulty-easy',   label: 'Easy' },
  'Medium': { cls: 'difficulty-medium', label: 'Medium' },
  'Hard':   { cls: 'difficulty-hard',   label: 'Hard' }
};

var isAdminUser      = false;
var editingProblemId = null;

var createForm       = document.getElementById('create-form');
var btnCreate        = document.getElementById('btn-create');
var btnCreateText    = document.getElementById('btn-create-text');
var btnCancelEdit    = document.getElementById('btn-cancel-edit');
var sectionLabel     = document.getElementById('create-section-label');
var formFeedback     = document.getElementById('form-feedback');
var testcaseContainer = document.getElementById('testcase-container');
var testcaseCount    = document.getElementById('testcase-count');
var btnAddTestcase   = document.getElementById('btn-add-testcase');
var createSection    = document.getElementById('create-section');
var listSection      = document.getElementById('list-section');

var adminLoading     = document.getElementById('admin-loading');
var adminError       = document.getElementById('admin-error');
var adminErrorText   = document.getElementById('admin-error-text');
var adminEmpty       = document.getElementById('admin-empty');
var adminTableWrap   = document.getElementById('admin-table-wrapper');
var adminTableBody   = document.getElementById('admin-table-body');

var adminDenied      = document.getElementById('admin-denied');
var adminContent     = document.getElementById('admin-content');

var testCaseCounter  = 0;

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
      isAdminUser = true;
    }
  }

  btnLogout.addEventListener('click', async function() {
    try { await api.logout(); } catch (e) {}
    auth.logout();
    window.location.href = '/login.html';
  });
}

function showDenied() {
  adminDenied.style.display = 'block';
  adminContent.style.display = 'none';
}

function showAdminContent() {
  adminDenied.style.display = 'none';
  adminContent.style.display = 'block';
}

function hideAdminStates() {
  adminLoading.classList.remove('visible');
  adminError.classList.remove('visible');
  adminEmpty.classList.remove('visible');
  adminTableWrap.style.display = 'none';
}

function showAdminLoading() {
  hideAdminStates();
  adminLoading.classList.add('visible');
}

function showAdminError(msg) {
  hideAdminStates();
  adminErrorText.textContent = msg || '加载失败';
  adminError.classList.add('visible');
}

function showAdminEmpty() {
  hideAdminStates();
  adminEmpty.classList.add('visible');
}

function showAdminTable(problems) {
  hideAdminStates();
  if (!problems || problems.length === 0) {
    showAdminEmpty();
    return;
  }

  adminTableBody.innerHTML = problems.map(function(p) {
    var diff = difficultyConfig[p.difficulty] || difficultyConfig['Easy'];
    return (
      '<tr data-id="' + p.id + '">' +
        '<td class="admin-col-id">' + p.id + '</td>' +
        '<td class="admin-col-title">' + escapeHtml(p.title) + '</td>' +
        '<td><span class="difficulty-badge ' + diff.cls + '">' + diff.label + '</span></td>' +
        '<td class="admin-col-actions">' +
          '<button class="btn-edit" onclick="startEdit(' + p.id + ')">' +
            '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" width="14" height="14">' +
              '<path d="M11 4H4a2 2 0 00-2 2v14a2 2 0 002 2h14a2 2 0 002-2v-7"/>' +
              '<path d="M18.5 2.5a2.121 2.121 0 013 3L12 15l-4 1 1-4 9.5-9.5z"/>' +
            '</svg>' +
            '编辑' +
          '</button>' +
          '<button class="btn-delete" onclick="confirmDelete(' + p.id + ', \'' + jsEscape(p.title) + '\')">' +
            '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" width="14" height="14">' +
              '<polyline points="3 6 5 6 21 6"/>' +
              '<path d="M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2"/>' +
            '</svg>' +
            '删除' +
          '</button>' +
        '</td>' +
      '</tr>'
    );
  }).join('');

  adminTableWrap.style.display = 'block';
}

async function loadAdminProblems() {
  showAdminLoading();
  try {
    var data = await api.getProblems();
    showAdminTable(data);
  } catch (err) {
    showAdminError(err.message);
  }
}

function escapeHtml(str) {
  var div = document.createElement('div');
  div.textContent = str;
  return div.innerHTML;
}

function jsEscape(str) {
  return str.replace(/\\/g, '\\\\').replace(/'/g, "\\'").replace(/"/g, '\\"');
}

/* ---- Test Cases ---- */

function createTestcaseRow(idx, inputVal, expectedVal) {
  var div = document.createElement('div');
  div.className = 'test-case-row';
  div.setAttribute('data-tc-index', idx);
  div.innerHTML =
    '<div class="form-group">' +
      '<label class="form-label">输入 #' + idx + '</label>' +
      '<textarea class="form-textarea tc-input" placeholder="例如：1 2" rows="2"></textarea>' +
    '</div>' +
    '<div class="form-group">' +
      '<label class="form-label">期望输出 #' + idx + '</label>' +
      '<textarea class="form-textarea tc-expected" placeholder="例如：3" rows="2"></textarea>' +
    '</div>' +
    '<button type="button" class="btn-remove-testcase" onclick="removeTestcase(' + idx + ')" title="移除此测试用例">' +
      '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">' +
        '<line x1="18" y1="6" x2="6" y2="18"/>' +
        '<line x1="6" y1="6" x2="18" y2="18"/>' +
      '</svg>' +
    '</button>';

  if (inputVal !== undefined) {
    div.querySelector('.tc-input').value = inputVal;
  }
  if (expectedVal !== undefined) {
    div.querySelector('.tc-expected').value = expectedVal;
  }

  return div;
}

function addTestcase(inputVal, expectedVal) {
  testCaseCounter++;
  var row = createTestcaseRow(testCaseCounter, inputVal, expectedVal);
  testcaseContainer.appendChild(row);
  updateTestcaseCount();
}

function removeTestcase(idx) {
  var row = testcaseContainer.querySelector('[data-tc-index="' + idx + '"]');
  if (row) {
    row.remove();
    updateTestcaseCount();
  }
}

function getTestcases() {
  var rows = testcaseContainer.querySelectorAll('.test-case-row');
  var cases = [];
  for (var i = 0; i < rows.length; i++) {
    var inputEl    = rows[i].querySelector('.tc-input');
    var expectedEl = rows[i].querySelector('.tc-expected');
    if (inputEl && expectedEl) {
      cases.push({
        input: inputEl.value,
        expected: expectedEl.value,
        position: i
      });
    }
  }
  return cases;
}

function updateTestcaseCount() {
  var count = testcaseContainer.querySelectorAll('.test-case-row').length;
  testcaseCount.textContent = count + ' 个';
}

function clearTestcases() {
  testcaseContainer.innerHTML = '';
  testCaseCounter = 0;
  updateTestcaseCount();
}

function populateTestcases(testCases) {
  clearTestcases();
  if (testCases && testCases.length > 0) {
    for (var i = 0; i < testCases.length; i++) {
      var tc = testCases[i];
      addTestcase(tc.input, tc.expected);
    }
  } else {
    addTestcase();
  }
}

/* ---- Section Toggle ---- */

function setupSectionToggle(section, toggleBtn) {
  toggleBtn.addEventListener('click', function() {
    section.classList.toggle('collapsed');
  });
}

/* ---- Form Feedback ---- */

function showFormFeedback(type, msg) {
  var icon = type === 'success'
    ? '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><polyline points="8 12 11 15 16 9"/></svg>'
    : '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><line x1="15" y1="9" x2="9" y2="15"/><line x1="9" y1="9" x2="15" y2="15"/></svg>';
  formFeedback.className = 'admin-form-feedback visible ' + type;
  formFeedback.innerHTML = icon + '<span>' + msg + '</span>';
}

function hideFormFeedback() {
  formFeedback.className = 'admin-form-feedback';
}

function setCreateLoading(state) {
  btnCreate.classList.toggle('loading', state);
  btnCreate.disabled = state;
}

/* ---- Edit Mode ---- */

function enterEditMode() {
  sectionLabel.textContent = '编辑题目';
  btnCreateText.textContent = '保存修改';
  btnCancelEdit.style.display = 'inline-flex';
  createSection.classList.remove('collapsed');
  createSection.scrollIntoView({ behavior: 'smooth' });
}

function exitEditMode() {
  editingProblemId = null;
  sectionLabel.textContent = '创建新题目';
  btnCreateText.textContent = '创建题目';
  btnCancelEdit.style.display = 'none';
  createForm.reset();
  clearTestcases();
  addTestcase();
  document.getElementById('problem-difficulty').value = 'Medium';
  document.getElementById('problem-solution-content').value = '';
  document.getElementById('problem-solution-code').value = '';
  hideFormFeedback();
}

async function startEdit(id) {
  try {
    var problem = await api.getProblem(id);
    populateEditForm(problem);
  } catch (err) {
    alert('无法加载题目: ' + (err.message || '未知错误'));
  }
}

function populateEditForm(problem) {
  editingProblemId = problem.id;
  document.getElementById('problem-title').value = problem.title;
  document.getElementById('problem-difficulty').value = problem.difficulty;
  document.getElementById('problem-content').value = problem.content;
  document.getElementById('problem-template').value = problem.code_template || '';
  document.getElementById('problem-solution-content').value = problem.solution_content || '';
  document.getElementById('problem-solution-code').value = problem.solution_code || '';
  populateTestcases(problem.test_cases || []);
  enterEditMode();
}

/* ---- Create / Update ---- */

createForm.addEventListener('submit', async function(e) {
  e.preventDefault();
  hideFormFeedback();

  var title = document.getElementById('problem-title').value.trim();
  var difficulty = document.getElementById('problem-difficulty').value;
  var content = document.getElementById('problem-content').value.trim();
  var template = document.getElementById('problem-template').value;
  var solutionContent = document.getElementById('problem-solution-content').value;
  var solutionCode = document.getElementById('problem-solution-code').value;

  if (!title) {
    showFormFeedback('error', '请输入题目标题');
    document.getElementById('problem-title').focus();
    return;
  }

  if (!content) {
    showFormFeedback('error', '请输入题目描述');
    document.getElementById('problem-content').focus();
    return;
  }

  var testCases = getTestcases();
  var payload = {
    title: title,
    difficulty: difficulty,
    content: content,
    template: template,
    solution_content: solutionContent,
    solution_code: solutionCode,
    test_cases: testCases
  };

  setCreateLoading(true);

  try {
    if (editingProblemId) {
      await api.updateProblem(editingProblemId, payload);
      showFormFeedback('success', '题目 #' + editingProblemId + ' 已更新！');
    } else {
      var data = await api.createProblem(payload);
      showFormFeedback('success', '题目 #' + data.id + ' 创建成功！');
    }

    exitEditMode();
    loadAdminProblems();
  } catch (err) {
    showFormFeedback('error', err.message || '操作失败');
  } finally {
    setCreateLoading(false);
  }
});

btnCancelEdit.addEventListener('click', function() {
  exitEditMode();
});

/* ---- Delete Problem ---- */

function confirmDelete(id, title) {
  if (!confirm('确定要删除题目 #' + id + '「' + title + '」吗？\n\n此操作不可撤销，题目及所有测试用例将被永久删除。')) {
    return;
  }
  deleteProblem(id);
}

async function deleteProblem(id) {
  var row = adminTableBody.querySelector('tr[data-id="' + id + '"]');
  if (row) {
    var btn = row.querySelector('.btn-delete');
    if (btn) btn.classList.add('deleting');
  }

  try {
    await api.deleteProblem(id);
    loadAdminProblems();
  } catch (err) {
    alert('删除失败: ' + (err.message || '未知错误'));
    if (row) {
      var btn = row.querySelector('.btn-delete');
      if (btn) btn.classList.remove('deleting');
    }
  }
}

/* ---- Init ---- */

function init() {
  setupNavbar();

  if (!auth.requireAuth()) return;

  var user = auth.getUser();
  if (!user || user.role !== 'admin') {
    showDenied();
    return;
  }

  showAdminContent();

  setupSectionToggle(createSection, document.getElementById('create-section-toggle'));
  setupSectionToggle(listSection, document.getElementById('list-section-toggle'));

  btnAddTestcase.addEventListener('click', function() { addTestcase(); });
  document.getElementById('btn-admin-retry').addEventListener('click', loadAdminProblems);

  addTestcase();
  loadAdminProblems();
}

document.addEventListener('DOMContentLoaded', init);
