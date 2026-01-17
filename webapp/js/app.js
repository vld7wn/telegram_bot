/**
 * Telegram WebApp - Заявка на подключение
 * Основной JavaScript модуль
 */

// ========== КОНФИГУРАЦИЯ ==========
const CONFIG = {
    // Данные загружаются из JSON файлов
    tradePoints: [],
    tariffs: []
};

// ========== ЗАГРУЗКА ДАННЫХ ==========
async function loadTradePoints() {
    try {
        const response = await fetch('data/trade_points.json');
        const data = await response.json();
        CONFIG.tradePoints = data.map(tp => ({
            code: tp.code,
            address: tp.address
        }));
    } catch (error) {
        console.error('Failed to load trade points:', error);
        // Fallback данные
        CONFIG.tradePoints = [{ code: 'DEFAULT', address: 'Офис МТС' }];
    }
}

async function loadTariffs() {
    try {
        const response = await fetch('data/tariffs.json');
        const data = await response.json();
        CONFIG.tariffs = data.map(t => ({
            id: t.id,
            name: t.name,
            speeds: t.speeds.map(s => ({
                value: s.value,
                unit: s.unit,
                price: s.price,
                fullPrice: s.full_price || s.price,
                promoMonths: s.promo_price_duration_months || ''
            })),
            features: buildFeatures(t),
            routerRental: t.router_rental || '',
            tvBoxRental: t.tv_box_rental || '',
            connectionFee: t.connection_fee || ''
        }));
    } catch (error) {
        console.error('Failed to load tariffs:', error);
    }
}

function buildFeatures(tariff) {
    const features = [];
    if (tariff.internet_unlimited) features.push('Безлимитный интернет');
    if (tariff.mobile_connection_included) {
        let mobile = 'Мобильная связь';
        if (tariff.mobile_internet_gb) mobile += ` (${tariff.mobile_internet_gb} ГБ`;
        if (tariff.mobile_minutes) mobile += `, ${tariff.mobile_minutes} мин`;
        if (tariff.mobile_sms) mobile += `, ${tariff.mobile_sms} SMS`;
        mobile += ')';
        features.push(mobile);
    }
    if (tariff.tv_kion) features.push('KION');
    if (tariff.tv_channels) features.push(tariff.tv_channels);
    return features.join(' • ') || 'Интернет';
}

// ========== СОСТОЯНИЕ ПРИЛОЖЕНИЯ ==========
const state = {
    currentScreen: 1,
    totalScreens: 5,

    // Выбранные данные
    tradePoint: null,
    tariff: null,
    speed: null,
    needsTvBox: false,

    // Данные пользователя
    userData: {
        name: '',
        phone: '',
        email: '',
        city: '',
        street: '',
        house: '',
        apartment: ''
    }
};

// ========== TELEGRAM WEBAPP ==========
const tg = window.Telegram?.WebApp;

function initTelegramWebApp() {
    if (tg) {
        tg.ready();
        tg.expand();

        // Применяем тему Telegram (опционально)
        // document.body.style.backgroundColor = tg.backgroundColor;

        // Настраиваем кнопку закрытия
        tg.BackButton.onClick(() => {
            if (state.currentScreen > 1) {
                goToScreen(state.currentScreen - 1);
            } else {
                tg.close();
            }
        });
    }
}

// ========== НАВИГАЦИЯ ==========
function goToScreen(screenNum) {
    if (screenNum < 1 || screenNum > state.totalScreens) return;

    // Скрываем текущий экран
    document.getElementById(`screen${state.currentScreen}`).classList.remove('active');

    // Показываем новый экран
    document.getElementById(`screen${screenNum}`).classList.add('active');

    state.currentScreen = screenNum;
    updateUI();
}

function updateUI() {
    // Обновляем прогресс-бар
    const progress = (state.currentScreen / state.totalScreens) * 100;
    document.getElementById('progressFill').style.width = `${progress}%`;

    // Обновляем заголовок шага
    const titles = [
        'Выбор точки',
        'Выбор тарифа',
        'Скорость и опции',
        'Ваши данные',
        'Подтверждение'
    ];
    document.getElementById('stepTitle').textContent =
        `Шаг ${state.currentScreen} из ${state.totalScreens}: ${titles[state.currentScreen - 1]}`;

    // Показываем/скрываем кнопку "Назад"
    const btnBack = document.getElementById('btnBack');
    btnBack.style.display = state.currentScreen > 1 ? 'flex' : 'none';

    // Меняем текст кнопки "Далее"
    const btnNext = document.getElementById('btnNext');
    if (state.currentScreen === state.totalScreens) {
        btnNext.innerHTML = 'Отправить заявку <span class="btn-icon">✓</span>';
    } else {
        btnNext.innerHTML = 'Далее <span class="btn-icon">→</span>';
    }

    // Telegram Back Button
    if (tg) {
        if (state.currentScreen > 1) {
            tg.BackButton.show();
        } else {
            tg.BackButton.hide();
        }
    }

    // Обновляем экран-специфичный UI
    if (state.currentScreen === 3) {
        updateSpeedOptionsScreen();
    } else if (state.currentScreen === 5) {
        updateSummaryScreen();
    }
}

// ========== ЭКРАН 1: ТОРГОВЫЕ ТОЧКИ ==========
function renderTradePoints() {
    const grid = document.getElementById('tradePointsGrid');
    grid.innerHTML = CONFIG.tradePoints.map((tp, index) => `
        <button class="trade-point-btn ${state.tradePoint?.code === tp.code ? 'selected' : ''}" 
                onclick="selectTradePoint(${index})">
            <span class="tp-code">${tp.code}</span>
            <span class="tp-address">${tp.address.substring(tp.address.indexOf(',') + 2)}</span>
        </button>
    `).join('');
}

function selectTradePoint(index) {
    state.tradePoint = CONFIG.tradePoints[index];
    renderTradePoints();
}

// ========== ЭКРАН 2: ТАРИФЫ ==========
function renderTariffs() {
    const list = document.getElementById('tariffsList');
    list.innerHTML = CONFIG.tariffs.map(tariff => {
        const minSpeed = tariff.speeds[0];
        return `
            <div class="glass-card tariff-card ${state.tariff?.id === tariff.id ? 'selected' : ''}" 
                 onclick="selectTariff('${tariff.id}')">
                <div class="tariff-name">${tariff.name}</div>
                <div class="tariff-speed">до ${minSpeed.value} ${minSpeed.unit}</div>
                <div class="tariff-price">
                    от <span class="amount">${minSpeed.price} ₽</span>/мес
                </div>
                <div class="tariff-features">${tariff.features}</div>
            </div>
        `;
    }).join('');
}

function selectTariff(tariffId) {
    state.tariff = CONFIG.tariffs.find(t => t.id === tariffId);
    state.speed = null; // Сбрасываем выбранную скорость
    renderTariffs();
}

// ========== ЭКРАН 3: СКОРОСТЬ И ОПЦИИ ==========
function updateSpeedOptionsScreen() {
    if (!state.tariff) return;

    // Выбранный тариф
    const card = document.getElementById('selectedTariffCard');
    card.innerHTML = `
        <div class="tariff-name">${state.tariff.name}</div>
        <div class="tariff-features">${state.tariff.features}</div>
    `;

    // Опции скоростей
    const speedOptions = document.getElementById('speedOptions');
    speedOptions.innerHTML = state.tariff.speeds.map((speed, index) => `
        <div class="glass-card speed-option ${state.speed === speed ? 'selected' : ''}" 
             onclick="selectSpeed(${index})">
            <span class="speed-value">${speed.value} ${speed.unit}</span>
            <span class="speed-price">${speed.price} ₽/мес</span>
        </div>
    `).join('');

    // ТВ-приставка
    const tvToggle = document.getElementById('tvBoxToggle');
    if (state.tariff.tvBoxRental && state.tariff.tvBoxRental !== '0 ₽/мес') {
        tvToggle.style.display = 'flex';
        document.getElementById('tvBoxPrice').textContent = state.tariff.tvBoxRental;
        document.getElementById('tvBoxCheckbox').checked = state.needsTvBox;
    } else {
        tvToggle.style.display = 'none';
    }
}

function selectSpeed(index) {
    state.speed = state.tariff.speeds[index];
    updateSpeedOptionsScreen();
}

// ========== ЭКРАН 5: ПОДТВЕРЖДЕНИЕ ==========
function updateSummaryScreen() {
    document.getElementById('summaryTariff').textContent = state.tariff?.name || '-';
    document.getElementById('summarySpeed').textContent =
        state.speed ? `${state.speed.value} ${state.speed.unit}` : '-';
    document.getElementById('summaryTvBox').textContent = state.needsTvBox ? 'Да' : 'Нет';
    document.getElementById('summaryName').textContent = state.userData.name || '-';
    document.getElementById('summaryPhone').textContent = state.userData.phone || '-';

    const address = [
        state.userData.city ? `г. ${state.userData.city}` : '',
        state.userData.street ? `ул. ${state.userData.street}` : '',
        state.userData.house ? `д. ${state.userData.house}` : '',
        state.userData.apartment ? `кв. ${state.userData.apartment}` : ''
    ].filter(Boolean).join(', ');
    document.getElementById('summaryAddress').textContent = address || '-';

    // Расчёт итоговой суммы
    let total = parseInt(state.speed?.price || 0);
    if (state.needsTvBox && state.tariff?.tvBoxRental) {
        const tvPrice = parseInt(state.tariff.tvBoxRental);
        if (!isNaN(tvPrice)) total += tvPrice;
    }
    document.getElementById('summaryTotal').textContent = `${total} ₽/мес`;
}

// ========== ВАЛИДАЦИЯ ==========
function validateCurrentScreen() {
    switch (state.currentScreen) {
        case 1:
            return state.tradePoint !== null;
        case 2:
            return state.tariff !== null;
        case 3:
            return state.speed !== null;
        case 4:
            return validateUserData();
        case 5:
            return true;
        default:
            return true;
    }
}

function validateUserData() {
    const { name, phone, email, city, street, house } = state.userData;
    return name.trim() && phone.trim() && email.trim() &&
        city.trim() && street.trim() && house.trim();
}

function collectUserData() {
    state.userData = {
        name: document.getElementById('userName').value.trim(),
        phone: document.getElementById('userPhone').value.trim(),
        email: document.getElementById('userEmail').value.trim(),
        city: document.getElementById('userCity').value.trim(),
        street: document.getElementById('userStreet').value.trim(),
        house: document.getElementById('userHouse').value.trim(),
        apartment: document.getElementById('userApartment').value.trim()
    };
}

// ========== ОТПРАВКА ДАННЫХ ==========
function submitApplication() {
    const data = {
        tradePoint: state.tradePoint?.code || '',
        tradePointAddress: state.tradePoint?.address || '',
        tariff: state.tariff.name,
        tariffId: state.tariff.id,
        speed: `${state.speed.value} ${state.speed.unit}`,
        price: state.speed.price,
        needsTvBox: state.needsTvBox,
        ...state.userData
    };

    if (tg) {
        // Отправляем данные в Telegram
        tg.sendData(JSON.stringify(data));
    } else {
        // Для тестирования без Telegram
        console.log('Application data:', data);
        alert('Заявка отправлена! (тестовый режим)');
    }
}

// ========== ОБРАБОТЧИКИ СОБЫТИЙ ==========
function setupEventListeners() {
    // Кнопка "Далее"
    document.getElementById('btnNext').addEventListener('click', () => {
        if (state.currentScreen === 4) {
            collectUserData();
        }

        if (!validateCurrentScreen()) {
            // TODO: показать ошибку валидации
            return;
        }

        if (state.currentScreen === state.totalScreens) {
            submitApplication();
        } else {
            goToScreen(state.currentScreen + 1);
        }
    });

    // Кнопка "Назад"
    document.getElementById('btnBack').addEventListener('click', () => {
        goToScreen(state.currentScreen - 1);
    });

    // ТВ-приставка toggle
    document.getElementById('tvBoxCheckbox').addEventListener('change', (e) => {
        state.needsTvBox = e.target.checked;
    });
}

// ========== ИНИЦИАЛИЗАЦИЯ ==========
async function init() {
    initTelegramWebApp();

    // Загружаем данные из JSON
    await Promise.all([loadTradePoints(), loadTariffs()]);

    renderTradePoints();
    renderTariffs();
    setupEventListeners();
    updateUI();
}

// Запуск при загрузке страницы
document.addEventListener('DOMContentLoaded', init);
