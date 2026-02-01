// ========== ADMIN PANEL JAVASCRIPT ==========

// API Configuration
const API_BASE = window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1'
    ? 'http://localhost:8080/api'
    : 'https://telegram-bot-lqlw.onrender.com/api';

console.log('[Admin Panel] Using API:', API_BASE);

// State
let currentPage = 'dashboard';
let applications = [];
let admins = [];
let pendingRequests = [];
let tradePoints = [];
let tariffs = [];
const cacheVersion = '1.0.2';

let chatInterval = null;
let currentAppId = null;
let currentUserId = null;
let botActive = true;

// ========== INITIALIZATION ==========
// ========== INITIALIZATION ==========
document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM Content Loaded');
    try {
        // Очистка демо-данных перед загрузкой
        document.querySelectorAll('.stat-number').forEach(el => el.textContent = '0');

        initNavigation();
        updateTime();
        setInterval(updateTime, 1000);
        loadAllData();

        // Обработка Enter в чате
        const chatInput = document.getElementById('chatInput');
        if (chatInput) {
            chatInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter' && !e.shiftKey) {
                    e.preventDefault();
                    sendChatMessage();
                }
            });
        }
    } catch (e) {
        console.error('Initialization Error:', e);
        alert('Ошибка инициализации скрипта: ' + e.message);
    }
});

// Expose functions to global scope for HTML access
window.showDiscountModal = showDiscountModal;
window.applyDiscount = applyDiscount; // Ensure this is defined before usage if hoisting works, otherwise move this to bottom


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
    const pageTitleEl = document.getElementById('pageTitle');
    if (pageTitleEl) pageTitleEl.textContent = titles[pageName] || pageName;
    else {
        const mainTitle = document.querySelector('.main-title');
        if (mainTitle) mainTitle.textContent = titles[pageName] || pageName;
    }

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
                    <button class="btn-view" onclick="viewApplication(${app.id})" title="Просмотр">👁</button>
                    <button class="btn-message" onclick="openChat(${app.id})" title="Чат">💬</button>
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
        const speedsText = (t.speeds || []).map(s => s.speed).join(', ');
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
            const icon = toggleBtn.querySelector('.qa-icon') || toggleBtn.querySelector('.action-icon');
            const text = toggleBtn.querySelector('.qa-text') || toggleBtn.querySelector('span:last-child');
            if (icon) icon.textContent = '🔴';
            if (text) text.textContent = 'Выключить бота';
        }
        if (toggleSwitch) toggleSwitch.checked = true;
    } else {
        statusDot?.classList.remove('active');
        if (statusText) statusText.textContent = 'Бот отключен';
        if (toggleBtn) {
            const icon = toggleBtn.querySelector('.qa-icon') || toggleBtn.querySelector('.action-icon');
            const text = toggleBtn.querySelector('.qa-text') || toggleBtn.querySelector('span:last-child');
            if (icon) icon.textContent = '🟢';
            if (text) text.textContent = 'Включить бота';
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
    if (modalId === 'chatModal') {
        if (chatInterval) {
            clearInterval(chatInterval);
            chatInterval = null;
        }
        currentAppId = null;
        currentUserId = null;
    }
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

function openChat(appId) {
    const app = applications.find(a => a.id === appId);
    if (!app) return;

    currentAppId = app.id;
    currentUserId = app.userId;

    document.getElementById('chatAppId').textContent = app.id;
    document.getElementById('chatHistory').innerHTML = '<div class="chat-empty">Загрузка сообщений...</div>';
    document.getElementById('chatInput').value = '';

    loadChatHistory(app.id);

    // Заполняем список администраторов для подписи
    const adminSelect = document.getElementById('chatAdminSelect');
    if (adminSelect) {
        adminSelect.innerHTML = '<option value="">Без подписи</option>' +
            admins.map(adm => {
                const id = adm.userId || adm.user_id;
                const name = adm.name || 'Admin';
                const tp = adm.tradePoint || adm.trade_point || '-';
                return `<option value="${id}" data-name="${name}" data-tp="${tp}">${name} (${tp})</option>`;
            }).join('');

        // Загружаем сохраненный выбор
        const savedAdminId = localStorage.getItem('preferredAdminId');
        if (savedAdminId) adminSelect.value = savedAdminId;

        // Сохраняем при изменении
        adminSelect.onchange = () => localStorage.setItem('preferredAdminId', adminSelect.value);
    }

    if (chatInterval) clearInterval(chatInterval);
    chatInterval = setInterval(() => loadChatHistory(app.id), 300);

    showModal('chatModal');
}

// ========== CHAT LOGIC ==========
async function loadChatHistory(appId) {
    console.log(`[Chat] Loading history for app ${appId}...`);
    try {
        const response = await fetch(`${API_BASE}/messages/${appId}`);
        if (response.ok) {
            const history = await response.json();
            console.log(`[Chat] Received ${history.length} messages`);
            renderChat(history);
        } else {
            console.error(`[Chat] Failed to load history: ${response.status}`);
            const container = document.getElementById('chatHistory');
            if (container.querySelector('.chat-empty')) {
                container.innerHTML = `<div class="chat-empty" style="color: #ff4d4d;">Ошибка загрузки сообщений (Status: ${response.status}). Проверьте логи сервера.</div>`;
            }
        }
    } catch (error) {
        console.error('[Chat] Error loading chat history:', error);
        const container = document.getElementById('chatHistory');
        if (container.querySelector('.chat-empty')) {
            container.innerHTML = '<div class="chat-empty" style="color: #ff4d4d;">Ошибка подключения к API.</div>';
        }
    }
}

function renderChat(history) {
    const container = document.getElementById('chatHistory');
    if (!history || history.length === 0) {
        container.innerHTML = '<div class="chat-empty">История переписки пуста. Напишите клиенту первым!</div>';
        return;
    }

    try {
        const html = history.map(msg => {
            // Безопасный парсинг времени
            let timeStr = '--:--';
            try {
                if (msg.timestamp) {
                    // SQLite возвращает "YYYY-MM-DD HH:MM:SS" в UTC
                    // Чтобы JS корректно понял, можно заменить пробел на T
                    const dateObj = new Date(msg.timestamp.replace(' ', 'T'));
                    if (!isNaN(dateObj.getTime())) {
                        timeStr = dateObj.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
                    } else {
                        // Если парсинг не удался, пробуем вывести как есть
                        timeStr = msg.timestamp.split(' ')[1] || msg.timestamp;
                    }
                }
            } catch (e) {
                console.warn('[Chat] Date parse error:', e, msg.timestamp);
            }

            return `
                <div class="chat-msg ${msg.sender}">
                    ${(msg.text || '').replace(/\n/g, '<br>')}
                    <span class="chat-msg-time">${timeStr}</span>
                </div>
            `;
        }).join('');

        // Сохраняем позицию скролла
        const isAtBottom = container.scrollHeight - container.scrollTop <= container.clientHeight + 100;

        container.innerHTML = html;

        // Прокручиваем вниз, если были внизу или это первая загрузка
        if (isAtBottom) {
            container.scrollTop = container.scrollHeight;
        }
    } catch (error) {
        console.error('[Chat] Render error:', error);
        container.innerHTML = `<div class="chat-empty" style="color: #ff4d4d;">Ошибка отображения чата: ${error.message}</div>`;
    }
}

async function sendChatMessage() {
    const input = document.getElementById('chatInput');
    const text = input.value.trim();

    if (!text || !currentAppId || !currentUserId) return;

    const btn = document.querySelector('.btn-send');
    btn.disabled = true;

    try {
        let finalContext = text;
        const adminSelect = document.getElementById('chatAdminSelect');
        if (adminSelect && adminSelect.value) {
            const opt = adminSelect.options[adminSelect.selectedIndex];
            const name = opt.dataset.name;
            const tp = opt.dataset.tp;
            finalContext = `Сотрудник\n${name} ${tp}\n-----------------\n${text}`;
        }

        const response = await fetch(`${API_BASE}/messages/${currentAppId}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                userId: currentUserId,
                text: finalContext
            })
        });

        if (response.ok) {
            input.value = '';
            await loadChatHistory(currentAppId);
            const container = document.getElementById('chatHistory');
            container.scrollTop = container.scrollHeight;
        } else {
            alert('Ошибка при отправке сообщения');
        }
    } catch (error) {
        console.error('Error sending message:', error);
        alert('Ошибка подключения');
    } finally {
        btn.disabled = false;
        input.focus();
    }
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
    if (!app) return;

    console.log(`[Status] Updating app ${appId} to ${newStatus}...`);
    try {
        const response = await fetch(`${API_BASE}/applications/${appId}/status`, {
            method: 'PATCH',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ status: newStatus })
        });
        if (response.ok) {
            console.log(`[Status] App ${appId} updated successfully`);
            app.status = newStatus;
            renderApplicationsTable();
            renderRecentApplications();
            updateDashboard();
        } else {
            console.error(`[Status] Failed to update: ${response.status}`);
            const error = await response.json();
            alert(`Ошибка обновления статуса: ${error.error || response.statusText}`);
        }
    } catch (error) {
        console.error('[Status] Network error:', error);
        alert('Ошибка подключения при обновлении статуса');
    }
}

// ========== TRADE POINTS ACTIONS ==========
function showAddTradePointModal() {
    document.getElementById('tradePointModalTitle').textContent = 'Добавить торговую точку';
    document.getElementById('tradePointCode').value = '';
    document.getElementById('tradePointAddress').value = '';
    document.getElementById('tradePointCode').disabled = false;
    showModal('tradePointModal');
}

async function editTradePoint(code) {
    const tp = tradePoints.find(t => t.code === code);
    if (!tp) return;

    document.getElementById('tradePointModalTitle').textContent = 'Редактировать точку';
    document.getElementById('tradePointCode').value = tp.code;
    document.getElementById('tradePointAddress').value = tp.address || '';
    document.getElementById('tradePointCode').disabled = true;
    showModal('tradePointModal');
}

async function saveTradePoint() {
    const code = document.getElementById('tradePointCode').value;
    const address = document.getElementById('tradePointAddress').value;
    const isEdit = document.getElementById('tradePointCode').disabled;

    if (!code || !address) {
        alert('Заполните все поля');
        return;
    }

    try {
        const method = isEdit ? 'PUT' : 'POST';
        const url = isEdit ? `${API_BASE}/trade-points/${code}` : `${API_BASE}/trade-points`;

        const response = await fetch(url, {
            method: method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name: code, address: address })
        });

        if (response.ok) {
            await loadTradePoints();
            renderTradePoints();
            populateTradePointSelects();
            closeModal('tradePointModal');
        } else {
            const err = await response.json();
            alert('Ошибка при сохранении: ' + (err.error || 'неизвестная ошибка'));
        }
    } catch (error) {
        console.error('Error saving trade point:', error);
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
    document.getElementById('editTariffId').value = '';
    document.getElementById('tariffModalTitle').textContent = 'Добавить тариф';
    document.getElementById('editTariffTitle').value = '';
    document.getElementById('editTariffConnFee').value = '0';
    document.getElementById('editTariffRouter').value = '0';
    document.getElementById('editTariffTvBox').value = '0';
    document.getElementById('editTariffExtra').value = '';
    document.getElementById('editTariffMobileIncluded').checked = false;
    document.getElementById('editTariffMobileGb').value = '';
    document.getElementById('editTariffMobileMin').value = '';
    document.getElementById('editTariffMobileSms').value = '';

    document.getElementById('tariffSpeedsContainer').innerHTML = '';
    document.getElementById('tariffServicesContainer').innerHTML = '';

    addSpeedRow();
    showModal('tariffModal');
}

function addSpeedRow(value = '', unit = 'Мбит/с', price = '') {
    const container = document.getElementById('tariffSpeedsContainer');
    const div = document.createElement('div');
    div.className = 'speed-row';
    div.innerHTML = `
        <div class="form-group" style="flex: 2;">
            <label>Скорость</label>
            <div style="display: flex; gap: 5px;">
                <input type="text" class="form-input speed-val" value="${value}" style="flex: 1;">
                <select class="form-input speed-unit" style="flex: 0 0 100px; padding: 10px 5px;">
                    <option value="Мбит/с" ${unit === 'Мбит/с' ? 'selected' : ''}>Мбит/с</option>
                    <option value="Гбит/с" ${unit === 'Гбит/с' ? 'selected' : ''}>Гбит/с</option>
                </select>
            </div>
        </div>
        <div class="form-group" style="flex: 1;">
            <label>Цена (₽)</label>
            <input type="text" class="form-input speed-price" value="${price}">
        </div>
        <button class="btn btn-danger btn-sm" onclick="this.parentElement.remove()" style="margin-bottom: 0;">🗑</button>
    `;
    container.appendChild(div);
}

function addServiceRow(text = '') {
    const container = document.getElementById('tariffServicesContainer');
    const div = document.createElement('div');
    div.className = 'service-row';
    div.innerHTML = `
        <div class="form-group">
            <input type="text" class="form-input service-text" value="${text}" placeholder="Название услуги">
        </div>
        <button class="btn btn-danger btn-sm" onclick="this.parentElement.remove()">🗑</button>
    `;
    container.appendChild(div);
}

async function editTariff(id) {
    const t = tariffs.find(tar => tar.id === id);
    if (!t) return;

    document.getElementById('editTariffId').value = t.id;
    document.getElementById('tariffModalTitle').textContent = 'Редактировать тариф';
    document.getElementById('editTariffTitle').value = t.name || '';
    document.getElementById('editTariffConnFee').value = t.connectionFee || '0';
    document.getElementById('editTariffRouter').value = t.routerRental || '0';
    document.getElementById('editTariffTvBox').value = t.tvBoxRental || '0';
    document.getElementById('editTariffExtra').value = t.extraDetails || '';

    document.getElementById('editTariffMobileIncluded').checked = !!t.mobileIncluded;
    document.getElementById('editTariffMobileGb').value = t.mobileInternetGb || '';
    document.getElementById('editTariffMobileMin').value = t.mobileMinutes || '';
    document.getElementById('editTariffMobileSms').value = t.mobileSms || '';

    // Speeds
    const speedContainer = document.getElementById('tariffSpeedsContainer');
    speedContainer.innerHTML = '';
    if (t.speeds && t.speeds.length > 0) {
        t.speeds.forEach(s => {
            const parts = s.speed.split(' ');
            const val = parts[0];
            const unit = parts.length > 1 ? parts.slice(1).join(' ') : 'Мбит/с';
            addSpeedRow(val, unit, s.price);
        });
    } else {
        addSpeedRow();
    }

    // Services
    const serviceContainer = document.getElementById('tariffServicesContainer');
    serviceContainer.innerHTML = '';
    if (t.services && t.services.length > 0) {
        t.services.forEach(svc => addServiceRow(svc));
    }

    showModal('tariffModal');
}

async function saveTariff() {
    const id = document.getElementById('editTariffId').value;
    const name = document.getElementById('editTariffTitle').value;

    if (!name) {
        alert('Введите название тарифа');
        return;
    }

    const speeds = [];
    document.querySelectorAll('.speed-row').forEach(row => {
        const val = row.querySelector('.speed-val').value;
        const unit = row.querySelector('.speed-unit').value;
        const price = row.querySelector('.speed-price').value;
        if (val && price) {
            speeds.push({ speed: `${val} ${unit}`, price: price });
        }
    });

    const services = [];
    document.querySelectorAll('.service-text').forEach(input => {
        if (input.value.trim()) services.push(input.value.trim());
    });

    const data = {
        name: name,
        connectionFee: document.getElementById('editTariffConnFee').value,
        routerRental: document.getElementById('editTariffRouter').value,
        tvBoxRental: document.getElementById('editTariffTvBox').value,
        extraDetails: document.getElementById('editTariffExtra').value,
        mobileIncluded: document.getElementById('editTariffMobileIncluded').checked,
        mobileInternetGb: document.getElementById('editTariffMobileGb').value,
        mobileMinutes: document.getElementById('editTariffMobileMin').value,
        mobileSms: document.getElementById('editTariffMobileSms').value,
        speeds: speeds,
        services: services
    };

    try {
        const method = id ? 'PUT' : 'POST';
        const url = id ? `${API_BASE}/tariffs/${id}` : `${API_BASE}/tariffs`;

        const response = await fetch(url, {
            method: method,
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data)
        });

        if (response.ok) {
            console.log('[Admin Panel] Tariff saved successfully');
            await loadTariffs();
            renderTariffs();
            closeModal('tariffModal');
        } else {
            const errData = await response.json().catch(() => ({}));
            console.error('[Admin Panel] Save tariff failed:', response.status, errData);
            alert(`Ошибка при сохранении: ${errData.error || response.statusText || response.status}`);
        }
    } catch (error) {
        console.error('Error saving tariff:', error);
        alert('Ошибка при сохранении: Проверьте соединение с сервером');
    }
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

// ========== DISCOUNT ACTIONS ==========
function showDiscountModal() {
    console.log('Attempting to open discount modal');
    try {
        const list = document.getElementById('discountTariffList');
        if (!list) {
            alert('Ошибка: Элемент списка тарифов не найден!');
            return;
        }

        if (!tariffs || tariffs.length === 0) {
            console.warn('No tariffs loaded');
            list.innerHTML = '<p style="padding:10px">Нет загруженных тарифов</p>';
        } else {
            list.innerHTML = tariffs.map(t => `
                <div style="display: flex; align-items: center; gap: 10px; padding: 8px; border-bottom: 1px solid rgba(255,255,255,0.05);">
                    <input type="checkbox" class="discount-checkbox" value="${t.id}" id="chk_${t.id}">
                    <label for="chk_${t.id}" style="cursor: pointer; flex-grow: 1;">${t.name} (от ${t.speeds[0]?.price || '-'} ₽)</label>
                </div>
            `).join('');
        }

        const input = document.getElementById('discountPercent');
        if (input) input.value = '';

        showModal('discountModal');
    } catch (e) {
        console.error('Error in showDiscountModal:', e);
        alert('Ошибка при открытии окна: ' + e.message);
    }
}

async function applyDiscount() {
    const percent = parseInt(document.getElementById('discountPercent').value);
    if (!percent || percent <= 0 || percent > 100) {
        alert('Введите корректный процент скидки (1-100)');
        return;
    }

    const selectedIds = Array.from(document.querySelectorAll('.discount-checkbox:checked')).map(cb => cb.value);
    if (selectedIds.length === 0) {
        alert('Выберите хотя бы один тариф');
        return;
    }

    if (!confirm(`Применить скидку ${percent}% к выбранным тарифам (${selectedIds.length} шт)? Цена будет окончательно изменена.`)) {
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/tariffs/discount`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                percent: percent,
                tariffIds: selectedIds
            })
        });

        if (response.ok) {
            alert('Скидка успешно применена!');
            closeModal('discountModal');
            loadTariffs().then(renderTariffs);
        } else {
            const err = await response.json();
            alert('Ошибка: ' + (err.error || 'Не удалось применить скидку'));
        }
    } catch (e) {
        console.error(e);
        alert('Ошибка подключения к серверу');
    }
}

// Ensure functions are globally available
window.showDiscountModal = showDiscountModal;
window.applyDiscount = applyDiscount;

