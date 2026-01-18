// ========== ADMIN PANEL JAVASCRIPT ==========

// API Configuration
const API_BASE = 'http://localhost:8080/api';

// State
let currentPage = 'dashboard';
let applications = [];
let admins = [];
let pendingRequests = [];
let tradePoints = [];
let tariffs = [];
let botActive = true;

// ========== INITIALIZATION ==========
document.addEventListener('DOMContentLoaded', () => {
    initNavigation();
    updateTime();
    setInterval(updateTime, 1000);
    loadAllData();
});

function initNavigation() {
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(item => {
        item.addEventListener('click', (e) => {
            e.preventDefault();
            const page = item.dataset.page;
            showPage(page);
            closeSidebar();
        });
    });
}

// ========== MOBILE SIDEBAR ==========
function toggleSidebar() {
    const sidebar = document.querySelector('.sidebar');
    const overlay = document.getElementById('sidebarOverlay');
    const menuBtn = document.getElementById('menuToggle');

    sidebar.classList.toggle('open');
    overlay.classList.toggle('active');
    menuBtn.classList.toggle('active');
}

function closeSidebar() {
    const sidebar = document.querySelector('.sidebar');
    const overlay = document.getElementById('sidebarOverlay');
    const menuBtn = document.getElementById('menuToggle');

    sidebar.classList.remove('open');
    overlay.classList.remove('active');
    menuBtn.classList.remove('active');
}

function showPage(pageName) {
    document.querySelectorAll('.nav-item').forEach(item => {
        item.classList.toggle('active', item.dataset.page === pageName);
    });

    document.querySelectorAll('.page').forEach(page => {
        page.classList.remove('active');
    });
    document.getElementById(`page-${pageName}`).classList.add('active');

    const titles = {
        'dashboard': 'Главная',
        'applications': 'Заявки',
        'admins': 'Админы',
        'trade-points': 'Торговые Точки',
        'tariffs': 'Тарифы',
        'settings': 'Настройки'
    };
    document.getElementById('pageTitle').textContent = titles[pageName] || pageName;

    currentPage = pageName;
}

function updateTime() {
    const now = new Date();
    const time = now.toLocaleTimeString('ru-RU', { hour: '2-digit', minute: '2-digit' });
    const date = now.toLocaleDateString('ru-RU', { day: 'numeric', month: 'long', year: 'numeric' });
    document.getElementById('currentTime').textContent = `${time} | ${date}`;
}

// ========== DATA LOADING ==========
async function loadAllData() {
    console.log('[Admin Panel] Loading data from API...');

    const results = await Promise.allSettled([
        loadApplications(),
        loadAdmins(),
        loadTradePoints(),
        loadTariffs(),
        loadBotStatus(),
        loadStats()
    ]);

    results.forEach((result, index) => {
        const names = ['applications', 'admins', 'tradePoints', 'tariffs', 'botStatus', 'stats'];
        if (result.status === 'rejected') {
            console.warn(`[Admin Panel] Failed to load ${names[index]}:`, result.reason);
        }
    });

    console.log('[Admin Panel] Data loaded:', {
        applications: applications.length,
        admins: admins.length,
        tradePoints: tradePoints.length,
        tariffs: tariffs.length
    });

    renderAll();
}

async function loadApplications() {
    const response = await fetch(`${API_BASE}/applications`);
    if (!response.ok) throw new Error('Failed to load applications');
    applications = await response.json();
}

async function loadAdmins() {
    const response = await fetch(`${API_BASE}/admins`);
    if (!response.ok) throw new Error('Failed to load admins');
    const data = await response.json();
    admins = data.admins || [];
    pendingRequests = data.pending || [];
}

async function loadTradePoints() {
    const response = await fetch(`${API_BASE}/trade-points`);
    if (!response.ok) throw new Error('Failed to load trade points');
    tradePoints = await response.json();
}

async function loadTariffs() {
    const response = await fetch(`${API_BASE}/tariffs`);
    if (!response.ok) throw new Error('Failed to load tariffs');
    tariffs = await response.json();
}

async function loadBotStatus() {
    const response = await fetch(`${API_BASE}/status`);
    if (!response.ok) throw new Error('Failed to load bot status');
    const data = await response.json();
    botActive = data.active;
    updateBotStatusUI();
}

async function loadStats() {
    try {
        const response = await fetch(`${API_BASE}/stats`);
        if (response.ok) {
            const stats = await response.json();
            document.getElementById('statTotal').textContent = stats.totalApplications || 0;
            document.getElementById('statNew').textContent = stats.newToday || 0;
            document.getElementById('statInProgress').textContent = stats.inProgress || 0;
            document.getElementById('statCompleted').textContent = stats.completed || 0;
        }
    } catch (e) {
        console.warn('Could not load stats from API');
    }
}

function refreshData() {
    loadAllData();
}

// ========== RENDERING ==========
function renderAll() {
    updateDashboard();
    renderApplicationsTable();
    renderAdminsList();
    renderPendingRequests();
    renderTradePoints();
    renderTariffs();
    updateBotStatusUI();
    populateTradePointSelects();
}

function updateDashboard() {
    const total = applications.length;
    const newCount = applications.filter(a => (a.status || '').includes('Новая')).length;
    const inProgress = applications.filter(a => (a.status || '').includes('В работе')).length;
    const completed = applications.filter(a => (a.status || '').includes('Выполнена')).length;

    document.getElementById('statTotal').textContent = total;
    document.getElementById('statNew').textContent = newCount;
    document.getElementById('statInProgress').textContent = inProgress;
    document.getElementById('statCompleted').textContent = completed;

    renderRecentApplications();
    renderChart();
}

function renderRecentApplications() {
    const tbody = document.getElementById('recentApplicationsBody');
    if (!tbody) return;

    const recent = applications.slice(0, 5);

    if (recent.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; color: var(--text-secondary);">Нет заявок</td></tr>';
        return;
    }

    tbody.innerHTML = recent.map(app => `
        <tr>
            <td>${app.id}</td>
            <td>${app.name || '-'}</td>
            <td>${app.phone || '-'}</td>
            <td>${app.tariff || '-'}</td>
            <td><span class="status-badge ${getStatusClass(app.status)}">${app.status || 'Новая'}</span></td>
            <td>${app.date || '-'}</td>
        </tr>
    `).join('');
}

function renderApplicationsTable() {
    const tbody = document.getElementById('applicationsBody');
    if (!tbody) return;

    if (applications.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8" style="text-align: center; color: var(--text-secondary);">Нет заявок</td></tr>';
        return;
    }

    tbody.innerHTML = applications.map(app => `
        <tr>
            <td>${app.id}</td>
            <td>${app.name || '-'}</td>
            <td>${app.phone || '-'}</td>
            <td>${app.tariff || '-'}</td>
            <td>${app.address || '-'}</td>
            <td><span class="status-badge ${getStatusClass(app.status)}">${app.status || 'Новая'}</span></td>
            <td>${app.date || '-'}</td>
            <td>
                <div class="action-buttons">
                    <button class="btn-view" onclick="viewApplication(${app.id})">👁</button>
                    <button class="btn-message" onclick="openMessageModal(${app.userId})">💬</button>
                </div>
            </td>
        </tr>
    `).join('');
}

function getStatusClass(status) {
    if (!status) return 'new';
    if (status.includes('Новая')) return 'new';
    if (status.includes('В работе')) return 'in-progress';
    if (status.includes('Выполнена')) return 'completed';
    if (status.includes('Отменена')) return 'cancelled';
    return '';
}

function renderAdminsList() {
    const container = document.getElementById('adminsList');
    if (!container) return;

    if (admins.length === 0) {
        container.innerHTML = '<p style="color: var(--text-secondary);">Нет активных администраторов</p>';
        return;
    }

    container.innerHTML = admins.map(admin => `
        <div class="admin-card">
            <div class="admin-info">
                <div class="admin-avatar">👤</div>
                <div class="admin-details">
                    <h4>${admin.name || 'Без имени'}</h4>
                    <p>ID: ${admin.userId || admin.user_id} | Точка: ${admin.tradePoint || admin.trade_point || '-'}</p>
                </div>
            </div>
            <button class="btn btn-danger" onclick="deleteAdmin(${admin.userId || admin.user_id})">🗑</button>
        </div>
    `).join('');
}

function renderPendingRequests() {
    const container = document.getElementById('pendingRequests');
    if (!container) return;

    if (pendingRequests.length === 0) {
        container.innerHTML = '<p style="color: var(--text-secondary);">Нет ожидающих запросов</p>';
        return;
    }

    container.innerHTML = pendingRequests.map(req => `
        <div class="request-card">
            <h4>${req.name || 'Без имени'}</h4>
            <p>ID: ${req.userId || req.user_id} | Точка: ${req.tradePoint || req.trade_point || '-'}</p>
            <div class="request-actions">
                <button class="btn-approve" onclick="approveAdmin(${req.userId || req.user_id})">✅ Одобрить</button>
                <button class="btn-decline" onclick="declineAdmin(${req.userId || req.user_id})">❌ Отклонить</button>
            </div>
        </div>
    `).join('');
}

function renderTradePoints() {
    const container = document.getElementById('tradePointsList');
    if (!container) return;

    if (tradePoints.length === 0) {
        container.innerHTML = '<p style="color: var(--text-secondary);">Нет торговых точек</p>';
        return;
    }

    container.innerHTML = tradePoints.map(tp => `
        <div class="admin-card">
            <div class="admin-info">
                <div class="admin-avatar">📍</div>
                <div class="admin-details">
                    <h4>${tp.code}</h4>
                    <p>${tp.address}</p>
                </div>
            </div>
            <div class="action-buttons">
                <button class="btn btn-secondary" onclick="editTradePoint('${tp.code}')">✏️</button>
                <button class="btn btn-danger" onclick="deleteTradePoint('${tp.code}')">🗑</button>
            </div>
        </div>
    `).join('');
}

function renderTariffs() {
    const container = document.getElementById('tariffsList');
    if (!container) return;

    if (tariffs.length === 0) {
        container.innerHTML = '<p style="color: var(--text-secondary);">Нет тарифов</p>';
        return;
    }

    container.innerHTML = tariffs.map(t => {
        const speedsText = (t.speeds || []).map(s => `${s.value} ${s.unit || 'Мбит/с'}`).join(', ');
        const priceText = (t.speeds && t.speeds[0]) ? `${t.speeds[0].price} ₽` : '-';

        return `
            <div class="admin-card">
                <div class="admin-info">
                    <div class="admin-avatar">💰</div>
                    <div class="admin-details">
                        <h4>${t.name || t.id}</h4>
                        <p>От ${priceText} | Скорости: ${speedsText || '-'}</p>
                    </div>
                </div>
                <button class="btn btn-secondary" onclick="editTariff('${t.id}')">✏️ Редактировать</button>
            </div>
        `;
    }).join('');
}

function populateTradePointSelects() {
    const selects = document.querySelectorAll('#newAdminTradePoint, #filterTradePoint');
    selects.forEach(select => {
        if (!select) return;
        const currentValue = select.value;
        const isFilter = select.id === 'filterTradePoint';

        select.innerHTML = isFilter ? '<option value="">Все точки</option>' : '';
        select.innerHTML += tradePoints.map(tp =>
            `<option value="${tp.code}">${tp.code} - ${tp.address}</option>`
        ).join('');

        if (currentValue) select.value = currentValue;
    });
}

function renderChart() {
    const ctx = document.getElementById('applicationsChart');
    if (!ctx) return;

    if (window.applicationsChartInstance) {
        window.applicationsChartInstance.destroy();
    }

    // Generate data from real applications
    const last30Days = {};
    for (let i = 29; i >= 0; i--) {
        const date = new Date();
        date.setDate(date.getDate() - i);
        const key = date.toISOString().split('T')[0];
        last30Days[key] = 0;
    }

    applications.forEach(app => {
        if (app.date) {
            const dateKey = app.date.split(' ')[0];
            if (last30Days.hasOwnProperty(dateKey)) {
                last30Days[dateKey]++;
            }
        }
    });

    const labels = Object.keys(last30Days).map(d => {
        const date = new Date(d);
        return date.toLocaleDateString('ru-RU', { day: 'numeric', month: 'short' });
    });
    const data = Object.values(last30Days);

    window.applicationsChartInstance = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [{
                label: 'Заявки',
                data: data,
                borderColor: '#00d4ff',
                backgroundColor: 'rgba(0, 212, 255, 0.1)',
                fill: true,
                tension: 0.4,
                pointRadius: 0,
                pointHoverRadius: 6,
                pointHoverBackgroundColor: '#00d4ff'
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false }
            },
            scales: {
                x: {
                    grid: { color: 'rgba(255, 255, 255, 0.05)' },
                    ticks: { color: 'rgba(255, 255, 255, 0.5)', maxTicksLimit: 7 }
                },
                y: {
                    grid: { color: 'rgba(255, 255, 255, 0.05)' },
                    ticks: { color: 'rgba(255, 255, 255, 0.5)' },
                    beginAtZero: true
                }
            }
        }
    });
}

// ========== BOT STATUS ==========
function updateBotStatusUI() {
    const statusDot = document.querySelector('.status-dot');
    const statusText = document.querySelector('.status-text');
    const toggleBtn = document.getElementById('toggleBotBtn');
    const toggleSwitch = document.getElementById('botActiveToggle');

    if (botActive) {
        statusDot?.classList.add('active');
        if (statusText) statusText.textContent = 'Бот активен';
        if (toggleBtn) {
            toggleBtn.querySelector('.action-icon').textContent = '🔴';
            toggleBtn.querySelector('span:last-child').textContent = 'Выключить бота';
        }
        if (toggleSwitch) toggleSwitch.checked = true;
    } else {
        statusDot?.classList.remove('active');
        if (statusText) statusText.textContent = 'Бот отключен';
        if (toggleBtn) {
            toggleBtn.querySelector('.action-icon').textContent = '🟢';
            toggleBtn.querySelector('span:last-child').textContent = 'Включить бота';
        }
        if (toggleSwitch) toggleSwitch.checked = false;
    }
}

async function toggleBot() {
    botActive = !botActive;
    updateBotStatusUI();

    try {
        await fetch(`${API_BASE}/status`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ active: botActive })
        });
    } catch (error) {
        console.error('Error toggling bot status:', error);
    }
}

// ========== MODALS ==========
function showModal(modalId) {
    document.getElementById(modalId).classList.add('active');
}

function closeModal(modalId) {
    document.getElementById(modalId).classList.remove('active');
}

function showAddAdminModal() {
    document.getElementById('newAdminId').value = '';
    document.getElementById('newAdminName').value = '';
    showModal('addAdminModal');
}

function viewApplication(appId) {
    const app = applications.find(a => a.id === appId);
    if (!app) return;

    document.getElementById('modalAppId').textContent = app.id;
    document.getElementById('applicationDetails').innerHTML = `
        <div class="form-group">
            <label>Имя клиента</label>
            <p style="font-size: 16px;">${app.name || '-'}</p>
        </div>
        <div class="form-group">
            <label>Телефон</label>
            <p style="font-size: 16px;">${app.phone || '-'}</p>
        </div>
        <div class="form-group">
            <label>Email</label>
            <p style="font-size: 16px;">${app.email || '-'}</p>
        </div>
        <div class="form-group">
            <label>Тариф</label>
            <p style="font-size: 16px;">${app.tariff || '-'}</p>
        </div>
        <div class="form-group">
            <label>Адрес</label>
            <p style="font-size: 16px;">${app.address || '-'}</p>
        </div>
        <div class="form-group">
            <label>Статус</label>
            <select class="form-input" onchange="updateApplicationStatus(${app.id}, this.value)">
                <option value="Новая" ${app.status === 'Новая' ? 'selected' : ''}>Новая</option>
                <option value="В работе" ${app.status === 'В работе' ? 'selected' : ''}>В работе</option>
                <option value="Выполнена" ${app.status === 'Выполнена' ? 'selected' : ''}>Выполнена</option>
                <option value="Отменена" ${app.status === 'Отменена' ? 'selected' : ''}>Отменена</option>
            </select>
        </div>
        <div class="form-group">
            <label>Дата создания</label>
            <p style="font-size: 16px;">${app.date || '-'}</p>
        </div>
    `;

    showModal('applicationModal');
}

// ========== ADMIN ACTIONS ==========
async function addAdmin() {
    const userId = document.getElementById('newAdminId').value;
    const name = document.getElementById('newAdminName').value;
    const tradePoint = document.getElementById('newAdminTradePoint').value;

    if (!userId || !name || !tradePoint) {
        alert('Заполните все поля');
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/admins`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ userId: parseInt(userId), name, tradePoint })
        });

        if (response.ok) {
            await loadAdmins();
            renderAdminsList();
            closeModal('addAdminModal');
        } else {
            alert('Ошибка при добавлении админа');
        }
    } catch (error) {
        console.error('Error adding admin:', error);
        alert('Ошибка подключения к API');
    }
}

async function deleteAdmin(userId) {
    if (!confirm('Удалить этого администратора?')) return;

    try {
        await fetch(`${API_BASE}/admins/${userId}`, { method: 'DELETE' });
        await loadAdmins();
        renderAdminsList();
    } catch (error) {
        console.error('Error deleting admin:', error);
    }
}

async function approveAdmin(userId) {
    try {
        await fetch(`${API_BASE}/admins/${userId}/approve`, { method: 'POST' });
        await loadAdmins();
        renderAdminsList();
        renderPendingRequests();
    } catch (error) {
        console.error('Error approving admin:', error);
    }
}

async function declineAdmin(userId) {
    try {
        await fetch(`${API_BASE}/admins/${userId}/decline`, { method: 'POST' });
        await loadAdmins();
        renderPendingRequests();
    } catch (error) {
        console.error('Error declining admin:', error);
    }
}

// ========== APPLICATION ACTIONS ==========
async function updateApplicationStatus(appId, newStatus) {
    const app = applications.find(a => a.id === appId);
    if (app) {
        app.status = newStatus;

        try {
            await fetch(`${API_BASE}/applications/${appId}/status`, {
                method: 'PATCH',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ status: newStatus })
            });
        } catch (error) {
            console.error('Error updating status:', error);
        }

        renderApplicationsTable();
        renderRecentApplications();
        updateDashboard();
    }
}

function openMessageModal(userId) {
    alert(`Отправка сообщения пользователю ${userId}`);
}

function sendMessageToClient() {
    alert('Функция отправки сообщения будет реализована');
    closeModal('applicationModal');
}

// ========== TRADE POINTS ACTIONS ==========
function showAddTradePointModal() {
    showModal('addTradePointModal');
}

async function addTradePoint() {
    const code = document.getElementById('newTradePointCode')?.value;
    const address = document.getElementById('newTradePointAddress')?.value;

    if (!code || !address) {
        alert('Заполните все поля');
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/trade-points`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ code, address })
        });

        if (response.ok) {
            await loadTradePoints();
            renderTradePoints();
            closeModal('addTradePointModal');
        } else {
            alert('Ошибка при добавлении торговой точки');
        }
    } catch (error) {
        console.error('Error adding trade point:', error);
        alert('Ошибка подключения к API');
    }
}

function editTradePoint(code) {
    const tp = tradePoints.find(t => t.code === code);
    if (!tp) return;

    const newAddress = prompt('Введите новый адрес:', tp.address);
    if (newAddress && newAddress !== tp.address) {
        // TODO: Implement API call to update trade point
        alert('Редактирование торговых точек будет реализовано в API');
    }
}

async function deleteTradePoint(code) {
    if (!confirm(`Удалить торговую точку ${code}?`)) return;

    try {
        const response = await fetch(`${API_BASE}/trade-points/${code}`, { method: 'DELETE' });
        if (response.ok) {
            await loadTradePoints();
            renderTradePoints();
        } else {
            alert('Ошибка при удалении');
        }
    } catch (error) {
        console.error('Error deleting trade point:', error);
        alert('Ошибка подключения к API');
    }
}

// ========== TARIFFS ACTIONS ==========
function showAddTariffModal() {
    alert('Добавление тарифов будет реализовано');
}

function editTariff(tariffId) {
    const tariff = tariffs.find(t => t.id === tariffId);
    if (!tariff) return;

    alert(`Редактирование тарифа "${tariff.name}" будет реализовано`);
}

// ========== EXPORT (ЗАГРУЗИТЬ) ==========
function downloadExcel() {
    if (applications.length === 0) {
        alert('Нет данных для экспорта');
        return;
    }

    // Create CSV with BOM for Excel compatibility
    let csv = '\uFEFFID;Имя;Телефон;Email;Тариф;Адрес;Статус;Дата\n';
    applications.forEach(app => {
        csv += `${app.id};"${app.name || ''}";"${app.phone || ''}";"${app.email || ''}";"${app.tariff || ''}";"${app.address || ''}";"${app.status || ''}";"${app.date || ''}"\n`;
    });

    const blob = new Blob([csv], { type: 'application/vnd.ms-excel;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `applications_${new Date().toISOString().split('T')[0]}.xls`;
    link.click();
    URL.revokeObjectURL(url);
}

// Legacy function name for compatibility
function exportToExcel() {
    downloadExcel();
}

// ========== BROADCAST ==========
async function sendBroadcast() {
    const message = document.getElementById('broadcastMessage').value;
    if (!message.trim()) {
        alert('Введите сообщение');
        return;
    }

    if (!confirm('Отправить сообщение всем пользователям?')) return;

    try {
        await fetch(`${API_BASE}/broadcast`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ message })
        });
        alert('Рассылка отправлена!');
        document.getElementById('broadcastMessage').value = '';
    } catch (error) {
        console.error('Error sending broadcast:', error);
        alert('Ошибка при отправке.');
    }
}

// ========== SEARCH & FILTER ==========
document.getElementById('searchApplications')?.addEventListener('input', filterApplications);
document.getElementById('filterStatus')?.addEventListener('change', filterApplications);
document.getElementById('filterTradePoint')?.addEventListener('change', filterApplications);

function filterApplications() {
    const search = (document.getElementById('searchApplications')?.value || '').toLowerCase();
    const statusFilter = document.getElementById('filterStatus')?.value || '';

    const filtered = applications.filter(app => {
        const matchesSearch = !search ||
            (app.name || '').toLowerCase().includes(search) ||
            (app.phone || '').includes(search) ||
            (app.address || '').toLowerCase().includes(search);

        const matchesStatus = !statusFilter || app.status === statusFilter;

        return matchesSearch && matchesStatus;
    });

    const tbody = document.getElementById('applicationsBody');
    if (!tbody) return;

    if (filtered.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8" style="text-align: center; color: var(--text-secondary);">Ничего не найдено</td></tr>';
        return;
    }

    tbody.innerHTML = filtered.map(app => `
        <tr>
            <td>${app.id}</td>
            <td>${app.name || '-'}</td>
            <td>${app.phone || '-'}</td>
            <td>${app.tariff || '-'}</td>
            <td>${app.address || '-'}</td>
            <td><span class="status-badge ${getStatusClass(app.status)}">${app.status || 'Новая'}</span></td>
            <td>${app.date || '-'}</td>
            <td>
                <div class="action-buttons">
                    <button class="btn-view" onclick="viewApplication(${app.id})">👁</button>
                    <button class="btn-message" onclick="openMessageModal(${app.userId})">💬</button>
                </div>
            </td>
        </tr>
    `).join('');
}
