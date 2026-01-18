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
            // Close sidebar on mobile after navigation
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
    // Update navigation
    document.querySelectorAll('.nav-item').forEach(item => {
        item.classList.toggle('active', item.dataset.page === pageName);
    });

    // Update pages
    document.querySelectorAll('.page').forEach(page => {
        page.classList.remove('active');
    });
    document.getElementById(`page-${pageName}`).classList.add('active');

    // Update title
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
    try {
        // Try to load from API first
        const results = await Promise.allSettled([
            loadApplications(),
            loadAdmins(),
            loadTradePoints(),
            loadTariffs(),
            loadBotStatus(),
            loadStats()
        ]);

        // Check if all critical requests succeeded
        const allSucceeded = results.every(r => r.status === 'fulfilled');
        if (!allSucceeded) {
            console.warn('Some API requests failed, using partial data');
        }

        renderAll();
    } catch (error) {
        console.error('Error loading data:', error);
        // Load fallback demo data if API is not available
        await loadDemoData();
    }
}

async function loadDemoData() {
    // Demo data for testing without API
    applications = [
        { id: 1, name: 'Иван Петров', phone: '+7 999 123-45-67', tariff: 'Бизнес (100 Мбит/с)', address: 'г. Москва, ул. Ленина, д. 10, кв. 5', status: 'Новая', date: '2024-03-29 14:35', userId: 123456 },
        { id: 2, name: 'Ольга Сидорова', phone: '+7 912 345-67-89', tariff: 'Базовый (50 Мбит/с)', address: 'г. Москва, пр. Мира, д. 45', status: 'В работе', date: '2024-03-29 12:20', userId: 234567 },
        { id: 3, name: 'Алексей Смирнов', phone: '+7 903 987-65-43', tariff: 'Премиум (200 Мбит/с)', address: 'г. Москва, ул. Пушкина, д. 22', status: 'Выполнена', date: '2024-03-28 16:45', userId: 345678 },
        { id: 4, name: 'Мария Козлова', phone: '+7 926 111-22-33', tariff: 'Базовый (50 Мбит/с)', address: 'г. Москва, ул. Гагарина, д. 8', status: 'Новая', date: '2024-03-29 10:15', userId: 456789 },
        { id: 5, name: 'Дмитрий Волков', phone: '+7 915 444-55-66', tariff: 'Бизнес (100 Мбит/с)', address: 'г. Москва, ул. Чехова, д. 15', status: 'Отменена', date: '2024-03-27 09:30', userId: 567890 },
    ];

    admins = [
        { userId: 111222333, name: 'Администратор 1', tradePoint: 'ТП-001', status: 'active' },
        { userId: 222333444, name: 'Администратор 2', tradePoint: 'ТП-002', status: 'active' },
    ];

    pendingRequests = [
        { userId: 333444555, name: 'Новый Админ', tradePoint: 'ТП-003' },
    ];

    tradePoints = [
        { code: 'ТП-001', address: 'г. Москва, ул. Тверская, д. 1' },
        { code: 'ТП-002', address: 'г. Москва, ул. Арбат, д. 10' },
        { code: 'ТП-003', address: 'г. Москва, Кутузовский пр., д. 5' },
    ];

    tariffs = [
        { id: 'basic', name: 'Базовый', speeds: ['50 Мбит/с', '100 Мбит/с'], price: '590 ₽' },
        { id: 'business', name: 'Бизнес', speeds: ['100 Мбит/с', '200 Мбит/с'], price: '890 ₽' },
        { id: 'premium', name: 'Премиум', speeds: ['200 Мбит/с', '500 Мбит/с'], price: '1290 ₽' },
    ];

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
    // Calculate stats from local applications array (fallback if API stats not loaded)
    const total = applications.length;
    const newCount = applications.filter(a => (a.status || '').includes('Новая')).length;
    const inProgress = applications.filter(a => (a.status || '').includes('В работе')).length;
    const completed = applications.filter(a => (a.status || '').includes('Выполнена')).length;

    // Only update if values are currently 0 (not already set by loadStats)
    const statTotal = document.getElementById('statTotal');
    if (statTotal.textContent === '0' || statTotal.textContent === '156') {
        statTotal.textContent = total;
        document.getElementById('statNew').textContent = newCount;
        document.getElementById('statInProgress').textContent = inProgress;
        document.getElementById('statCompleted').textContent = completed;
    }

    renderRecentApplications();
    renderChart();
}

function renderRecentApplications() {
    const tbody = document.getElementById('recentApplicationsBody');
    const recent = applications.slice(0, 5);

    tbody.innerHTML = recent.map(app => `
        <tr>
            <td>${app.id}</td>
            <td>${app.name}</td>
            <td>${app.phone}</td>
            <td>${app.tariff}</td>
            <td><span class="status-badge ${getStatusClass(app.status)}">${app.status}</span></td>
            <td>${app.date}</td>
        </tr>
    `).join('');
}

function renderApplicationsTable() {
    const tbody = document.getElementById('applicationsBody');

    tbody.innerHTML = applications.map(app => `
        <tr>
            <td>${app.id}</td>
            <td>${app.name}</td>
            <td>${app.phone}</td>
            <td>${app.tariff}</td>
            <td>${app.address}</td>
            <td><span class="status-badge ${getStatusClass(app.status)}">${app.status}</span></td>
            <td>${app.date}</td>
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
    switch (status) {
        case 'Новая': return 'new';
        case 'В работе': return 'in-progress';
        case 'Выполнена': return 'completed';
        case 'Отменена': return 'cancelled';
        default: return '';
    }
}

function renderAdminsList() {
    const container = document.getElementById('adminsList');

    if (admins.length === 0) {
        container.innerHTML = '<p style="color: var(--text-secondary);">Нет активных администраторов</p>';
        return;
    }

    container.innerHTML = admins.map(admin => `
        <div class="admin-card">
            <div class="admin-info">
                <div class="admin-avatar">👤</div>
                <div class="admin-details">
                    <h4>${admin.name}</h4>
                    <p>ID: ${admin.userId} | Точка: ${admin.tradePoint}</p>
                </div>
            </div>
            <button class="btn btn-danger" onclick="deleteAdmin(${admin.userId})">🗑</button>
        </div>
    `).join('');
}

function renderPendingRequests() {
    const container = document.getElementById('pendingRequests');

    if (pendingRequests.length === 0) {
        container.innerHTML = '<p style="color: var(--text-secondary);">Нет ожидающих запросов</p>';
        return;
    }

    container.innerHTML = pendingRequests.map(req => `
        <div class="request-card">
            <h4>${req.name}</h4>
            <p>ID: ${req.userId} | Точка: ${req.tradePoint}</p>
            <div class="request-actions">
                <button class="btn-approve" onclick="approveAdmin(${req.userId})">✅ Одобрить</button>
                <button class="btn-decline" onclick="declineAdmin(${req.userId})">❌ Отклонить</button>
            </div>
        </div>
    `).join('');
}

function renderTradePoints() {
    const container = document.getElementById('tradePointsList');

    container.innerHTML = tradePoints.map(tp => `
        <div class="admin-card">
            <div class="admin-info">
                <div class="admin-avatar">📍</div>
                <div class="admin-details">
                    <h4>${tp.code}</h4>
                    <p>${tp.address}</p>
                </div>
            </div>
            <button class="btn btn-danger" onclick="deleteTradePoint('${tp.code}')">🗑</button>
        </div>
    `).join('');
}

function renderTariffs() {
    const container = document.getElementById('tariffsList');

    container.innerHTML = tariffs.map(t => `
        <div class="admin-card">
            <div class="admin-info">
                <div class="admin-avatar">💰</div>
                <div class="admin-details">
                    <h4>${t.name}</h4>
                    <p>От ${t.price} | Скорости: ${t.speeds.join(', ')}</p>
                </div>
            </div>
            <button class="btn btn-secondary" onclick="editTariff('${t.id}')">✏️ Редактировать</button>
        </div>
    `).join('');
}

function populateTradePointSelects() {
    const selects = document.querySelectorAll('#newAdminTradePoint, #filterTradePoint');
    selects.forEach(select => {
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

    // Destroy existing chart if any
    if (window.applicationsChartInstance) {
        window.applicationsChartInstance.destroy();
    }

    // Generate last 30 days labels
    const labels = [];
    const data = [];
    for (let i = 29; i >= 0; i--) {
        const date = new Date();
        date.setDate(date.getDate() - i);
        labels.push(date.toLocaleDateString('ru-RU', { day: 'numeric', month: 'short' }));
        data.push(Math.floor(Math.random() * 10) + 1); // Demo data
    }

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
                legend: {
                    display: false
                }
            },
            scales: {
                x: {
                    grid: {
                        color: 'rgba(255, 255, 255, 0.05)'
                    },
                    ticks: {
                        color: 'rgba(255, 255, 255, 0.5)',
                        maxTicksLimit: 7
                    }
                },
                y: {
                    grid: {
                        color: 'rgba(255, 255, 255, 0.05)'
                    },
                    ticks: {
                        color: 'rgba(255, 255, 255, 0.5)'
                    },
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
            <p style="font-size: 16px;">${app.name}</p>
        </div>
        <div class="form-group">
            <label>Телефон</label>
            <p style="font-size: 16px;">${app.phone}</p>
        </div>
        <div class="form-group">
            <label>Тариф</label>
            <p style="font-size: 16px;">${app.tariff}</p>
        </div>
        <div class="form-group">
            <label>Адрес</label>
            <p style="font-size: 16px;">${app.address}</p>
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
            <p style="font-size: 16px;">${app.date}</p>
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
        await fetch(`${API_BASE}/admins`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ userId: parseInt(userId), name, tradePoint })
        });

        admins.push({ userId: parseInt(userId), name, tradePoint, status: 'active' });
        renderAdminsList();
        closeModal('addAdminModal');
    } catch (error) {
        console.error('Error adding admin:', error);
        // Demo mode: add locally
        admins.push({ userId: parseInt(userId), name, tradePoint, status: 'active' });
        renderAdminsList();
        closeModal('addAdminModal');
    }
}

async function deleteAdmin(userId) {
    if (!confirm('Удалить этого администратора?')) return;

    try {
        await fetch(`${API_BASE}/admins/${userId}`, { method: 'DELETE' });
    } catch (error) {
        console.error('Error deleting admin:', error);
    }

    admins = admins.filter(a => a.userId !== userId);
    renderAdminsList();
}

async function approveAdmin(userId) {
    try {
        await fetch(`${API_BASE}/admins/${userId}/approve`, { method: 'POST' });
    } catch (error) {
        console.error('Error approving admin:', error);
    }

    const request = pendingRequests.find(r => r.userId === userId);
    if (request) {
        admins.push({ ...request, status: 'active' });
        pendingRequests = pendingRequests.filter(r => r.userId !== userId);
        renderAdminsList();
        renderPendingRequests();
    }
}

async function declineAdmin(userId) {
    try {
        await fetch(`${API_BASE}/admins/${userId}/decline`, { method: 'POST' });
    } catch (error) {
        console.error('Error declining admin:', error);
    }

    pendingRequests = pendingRequests.filter(r => r.userId !== userId);
    renderPendingRequests();
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
    // TODO: Implement message sending
    alert(`Отправка сообщения пользователю ${userId}`);
}

function sendMessageToClient() {
    // TODO: Implement message sending
    alert('Функция отправки сообщения будет реализована с HTTP API');
    closeModal('applicationModal');
}

// ========== EXPORT ==========
function exportToExcel() {
    // Create CSV content
    let csv = 'ID,Имя,Телефон,Тариф,Адрес,Статус,Дата\n';
    applications.forEach(app => {
        csv += `${app.id},"${app.name}","${app.phone}","${app.tariff}","${app.address}","${app.status}","${app.date}"\n`;
    });

    // Download
    const blob = new Blob(['\ufeff' + csv], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = `applications_${new Date().toISOString().split('T')[0]}.csv`;
    link.click();
    URL.revokeObjectURL(url);
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
        alert('Ошибка при отправке. Проверьте подключение к API.');
    }
}

// ========== SEARCH & FILTER ==========
document.getElementById('searchApplications')?.addEventListener('input', filterApplications);
document.getElementById('filterStatus')?.addEventListener('change', filterApplications);
document.getElementById('filterTradePoint')?.addEventListener('change', filterApplications);

function filterApplications() {
    const search = document.getElementById('searchApplications').value.toLowerCase();
    const statusFilter = document.getElementById('filterStatus').value;

    const filtered = applications.filter(app => {
        const matchesSearch = !search ||
            app.name.toLowerCase().includes(search) ||
            app.phone.includes(search) ||
            app.address.toLowerCase().includes(search);

        const matchesStatus = !statusFilter || app.status === statusFilter;

        return matchesSearch && matchesStatus;
    });

    const tbody = document.getElementById('applicationsBody');
    tbody.innerHTML = filtered.map(app => `
        <tr>
            <td>${app.id}</td>
            <td>${app.name}</td>
            <td>${app.phone}</td>
            <td>${app.tariff}</td>
            <td>${app.address}</td>
            <td><span class="status-badge ${getStatusClass(app.status)}">${app.status}</span></td>
            <td>${app.date}</td>
            <td>
                <div class="action-buttons">
                    <button class="btn-view" onclick="viewApplication(${app.id})">👁</button>
                    <button class="btn-message" onclick="openMessageModal(${app.userId})">💬</button>
                </div>
            </td>
        </tr>
    `).join('');
}
