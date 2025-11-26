const FALLBACK_API_BASE = "http://localhost:8080";

function padNumber(value) {
  return value.toString().padStart(2, "0");
}

function formatIdentifier(prefix, number, width = 4) {
  return `${prefix}${number.toString().padStart(width, "0")}`;
}

function formatDateTimeValue(date) {
  return `${date.getFullYear()}-${padNumber(date.getMonth() + 1)}-${padNumber(date.getDate())} ${padNumber(
    date.getHours()
  )}:${padNumber(date.getMinutes())}`;
}

function formatDateValue(date) {
  return `${date.getFullYear()}-${padNumber(date.getMonth() + 1)}-${padNumber(date.getDate())}`;
}

function addMinutes(date, minutes) {
  return new Date(date.getTime() + minutes * 60000);
}

function parseDateTimeInput(value) {
  if (!value || typeof value !== "string") {
    return null;
  }
  const match = value.trim().match(/^(\d{4})-(\d{2})-(\d{2})\s+(\d{2}):(\d{2})$/);
  if (!match) {
    return null;
  }
  const [, year, month, day, hour, minute] = match.map(Number);
  const parsed = new Date(year, month - 1, day, hour, minute, 0, 0);
  return Number.isNaN(parsed.getTime()) ? null : parsed;
}

function supportsLocalStorage() {
  try {
    const key = "__storage_test__";
    window.localStorage.setItem(key, "1");
    window.localStorage.removeItem(key);
    return true;
  } catch (error) {
    return false;
  }
}

function deepClone(data) {
  return JSON.parse(JSON.stringify(data));
}

function normalizeApiPath(url) {
  if (/^https?:\/\//i.test(url)) {
    try {
      const parsed = new URL(url);
      return `${parsed.pathname}${parsed.search}` || parsed.pathname;
    } catch (error) {
      return url;
    }
  }
  return url;
}

const STORAGE_AVAILABLE = supportsLocalStorage();
const USING_FILE_ORIGIN = window.location.protocol === "file:" || window.location.origin === "null";
const USE_MOCK_API = false;

const runtimeConfiguredApiBase =
  typeof window !== "undefined" && window.__BOOKING_API_BASE__ ? window.__BOOKING_API_BASE__ : null;
const queryConfiguredApiBase = (() => {
  try {
    const params = new URLSearchParams(window.location.search || "");
    const value = params.get("api") || params.get("apiBase");
    return value && value.trim().length > 0 ? value.trim() : null;
  } catch (error) {
    return null;
  }
})();

function readStoredApiBase() {
  if (!STORAGE_AVAILABLE) {
    return null;
  }
  try {
    return window.localStorage.getItem("restaurant-booking-api-base");
  } catch (error) {
    return null;
  }
}

function persistApiBase(base) {
  if (!STORAGE_AVAILABLE || !base) {
    return;
  }
  try {
    window.localStorage.setItem("restaurant-booking-api-base", base);
  } catch (error) {
    // 忽略存储异常
  }
}

const apiBase = (() => {
  if (runtimeConfiguredApiBase) return runtimeConfiguredApiBase;
  if (queryConfiguredApiBase) {
    persistApiBase(queryConfiguredApiBase);
    return queryConfiguredApiBase;
  }
  const stored = readStoredApiBase();
  if (stored) return stored;
  if (!USING_FILE_ORIGIN && window.location.origin) {
    return ""; // 与后端同源，直接使用相对路径
  }
  return FALLBACK_API_BASE;
})();

async function ensureApiBase() {
  return apiBase;
}

function createMockApi() {
  const storageKey = "restaurant-booking-demo-state";
  const defaultDuration = 120;
  const hasStorage = STORAGE_AVAILABLE;

  function buildDefaultState() {
    const now = new Date();
    const upcoming = (() => {
      const base = new Date(now.getTime());
      base.setHours(18, 0, 0, 0);
      if (base <= now) {
        base.setDate(base.getDate() + 1);
      }
      return base;
    })();

    const inProgress = new Date(now.getTime() - 45 * 60000);
    const menu = [
      { name: "Seared Salmon", category: "主菜", price: 24.5 },
      { name: "Garden Salad", category: "前菜", price: 8.5 },
      { name: "Ribeye Steak", category: "主菜", price: 36.0 },
      { name: "Tiramisu", category: "甜品", price: 7.5 },
      { name: "Fresh Lemonade", category: "饮品", price: 4.5 },
    ];

    const reservations = [
      {
        id: "R1001",
        customer: "王小明",
        phone: "13800000001",
        email: "",
        preference: "靠窗位",
        partySize: 2,
        time: formatDateTimeValue(inProgress),
        endTime: formatDateTimeValue(addMinutes(inProgress, defaultDuration)),
        durationMinutes: defaultDuration,
        status: "Seated",
        notes: "提前点了甜品",
        tableId: 1,
        lastModified: formatDateTimeValue(now),
      },
      {
        id: "R1002",
        customer: "李梅",
        phone: "13800000002",
        email: "limei@example.com",
        preference: "不辣",
        partySize: 4,
        time: formatDateTimeValue(upcoming),
        endTime: formatDateTimeValue(addMinutes(upcoming, defaultDuration)),
        durationMinutes: defaultDuration,
        status: "Open",
        notes: "",
        tableId: 3,
        lastModified: formatDateTimeValue(now),
      },
    ];

    const sampleOrderItems = [
      { name: menu[0].name, category: menu[0].category, price: menu[0].price, quantity: 1 },
      { name: menu[4].name, category: menu[4].category, price: menu[4].price, quantity: 2 },
    ].map((item) => ({
      ...item,
      lineTotal: Math.round(item.price * item.quantity * 100) / 100,
    }));
    const sampleOrderTotal = sampleOrderItems.reduce((sum, item) => sum + item.lineTotal, 0);

    return {
      tables: [
        { id: 1, capacity: 2, location: "Window", status: "Free" },
        { id: 2, capacity: 2, location: "Window", status: "Free" },
        { id: 3, capacity: 4, location: "Center", status: "Free" },
        { id: 4, capacity: 4, location: "Center", status: "Free" },
        { id: 5, capacity: 6, location: "Patio", status: "Free" },
      ],
      reservations,
      orders: [
        {
          id: "O5001",
          reservationId: "R1001",
          items: sampleOrderItems,
          total: Math.round(sampleOrderTotal * 100) / 100,
        },
      ],
      menu,
      staff: [
        { name: "Alice", role: "前台", contact: "alice@example.com" },
        { name: "Bob", role: "前台", contact: "bob@example.com" },
        { name: "Grace", role: "经理", contact: "grace@example.com" },
      ],
      bookingDate: formatDateValue(now),
      nextReservationNumber: 1003,
      nextWalkInNumber: 5001,
      nextOrderNumber: 5002,
    };
  }

  function hydrateState() {
    if (!hasStorage) {
      return buildDefaultState();
    }
    try {
      const stored = window.localStorage.getItem(storageKey);
      if (!stored) {
        return buildDefaultState();
      }
      const parsed = JSON.parse(stored);
      if (!parsed || typeof parsed !== "object") {
        return buildDefaultState();
      }
      const requiredArrays = ["tables", "reservations", "orders", "menu", "staff"];
      for (const key of requiredArrays) {
        if (!Array.isArray(parsed[key])) {
          return buildDefaultState();
        }
      }
      if (typeof parsed.nextReservationNumber !== "number" || typeof parsed.nextOrderNumber !== "number") {
        parsed.nextReservationNumber = 1000 + parsed.reservations.length + 1;
        parsed.nextOrderNumber = 5000 + parsed.orders.length + 1;
      }
      if (typeof parsed.nextWalkInNumber !== "number") {
        const walkInCount = parsed.reservations.filter((item) => typeof item.id === "string" && item.id.startsWith("W")).length;
        parsed.nextWalkInNumber = 5000 + walkInCount + 1;
      }
      if (typeof parsed.bookingDate !== "string") {
        parsed.bookingDate = formatDateValue(new Date());
      }
      return parsed;
    } catch (error) {
      return buildDefaultState();
    }
  }

  let state = hydrateState();

  function persist() {
    if (!hasStorage) {
      return;
    }
    try {
      window.localStorage.setItem(storageKey, JSON.stringify(state));
    } catch (error) {
      // ignore storage errors silently for offline demo
    }
  }

  function findTableById(id) {
    return state.tables.find((table) => table.id === id) || null;
  }

  function isTableAvailable(tableId, start, durationMinutes, ignoreReservationId) {
    const end = addMinutes(start, durationMinutes);
    return !state.reservations.some((reservation) => {
      if (!reservation.tableId || reservation.tableId !== tableId) {
        return false;
      }
      if (reservation.status === "Cancelled") {
        return false;
      }
      if (ignoreReservationId && reservation.id === ignoreReservationId) {
        return false;
      }
      const existingStart = parseDateTimeInput(reservation.time);
      if (!existingStart) {
        return false;
      }
      const existingEnd = addMinutes(existingStart, reservation.durationMinutes || defaultDuration);
      return !(end <= existingStart || start >= existingEnd);
    });
  }

  function findAvailableTable(partySize, start, durationMinutes, ignoreReservationId) {
    const candidates = state.tables
      .filter((table) => table.capacity >= partySize && table.status !== "OutOfService")
      .sort((a, b) => a.capacity - b.capacity || a.id - b.id);
    for (const table of candidates) {
      if (isTableAvailable(table.id, start, durationMinutes, ignoreReservationId)) {
        return table.id;
      }
    }
    return null;
  }

  function recomputeTables() {
    const now = new Date();
    state.tables.forEach((table) => {
      if (table.status !== "OutOfService") {
        table.status = "Free";
      }
    });
    state.reservations.forEach((reservation) => {
      if (!reservation.tableId || reservation.status === "Cancelled") {
        return;
      }
      const table = findTableById(reservation.tableId);
      if (!table || table.status === "OutOfService") {
        return;
      }
      if (reservation.status === "Completed") {
        return;
      }
      const start = parseDateTimeInput(reservation.time);
      if (!start) {
        return;
      }
      const end = addMinutes(start, reservation.durationMinutes || defaultDuration);
      if (reservation.status === "Seated" || (now >= start && now < end)) {
        table.status = "Occupied";
      } else if (now < start) {
        table.status = "Reserved";
      }
    });
  }

  function makeJsonResponse(data, status = 200) {
    const clone = deepClone(data);
    const text = JSON.stringify(clone);
    return {
      ok: status >= 200 && status < 300,
      status,
      json: async () => clone,
      text: async () => text,
    };
  }

  function makeTextResponse(message, status = 400) {
    const text = message ?? "";
    return {
      ok: status >= 200 && status < 300,
      status,
      json: async () => ({ message: text }),
      text: async () => text,
    };
  }

  function getFormField(map, key) {
    const value = map.get(key);
    return Array.isArray(value) ? value[0] : value;
  }

  function parseFormBody(body) {
    const params = new URLSearchParams(body || "");
    const map = new Map();
    for (const [key, value] of params.entries()) {
      if (map.has(key)) {
        map.get(key).push(value);
      } else {
        map.set(key, [value]);
      }
    }
    return map;
  }

  function getReservationById(id) {
    return state.reservations.find((item) => item.id === id) || null;
  }

  function removeOrdersByReservation(id) {
    state.orders = state.orders.filter((order) => order.reservationId !== id);
  }

  function serializeTables() {
    return state.tables.map((table) => {
      const reservations = state.reservations
        .filter((reservation) => reservation.tableId === table.id && reservation.status !== "Cancelled")
        .map((reservation) => ({
          id: reservation.id,
          customer: reservation.customer,
          partySize: reservation.partySize,
          status: reservation.status,
          orders: state.orders
            .filter((order) => order.reservationId === reservation.id)
            .map((order) => order.id),
        }));
      return { ...table, reservations };
    });
  }

  function assignTableManually(reservation, tableId) {
    const table = findTableById(tableId);
    if (!table) {
      return { ok: false, status: 404, message: "桌位不存在" };
    }
    const start = parseDateTimeInput(reservation.time);
    if (!start) {
      return { ok: false, status: 400, message: "预订时间无效" };
    }
    if (table.capacity < reservation.partySize) {
      return { ok: false, status: 409, message: "桌位容量不足" };
    }
    if (!isTableAvailable(tableId, start, reservation.durationMinutes || defaultDuration, reservation.id)) {
      return { ok: false, status: 409, message: "桌位已被占用" };
    }
    reservation.tableId = tableId;
    reservation.lastModified = formatDateTimeValue(new Date());
    recomputeTables();
    persist();
    return { ok: true };
  }

  function autoAssignTableForReservation(reservation) {
    const start = parseDateTimeInput(reservation.time);
    if (!start) {
      return { ok: false, status: 400, message: "预订时间无效" };
    }
    const tableId = findAvailableTable(
      reservation.partySize,
      start,
      reservation.durationMinutes || defaultDuration,
      reservation.id
    );
    if (!tableId) {
      return { ok: false, status: 409, message: "暂无可用桌位" };
    }
    reservation.tableId = tableId;
    reservation.lastModified = formatDateTimeValue(new Date());
    recomputeTables();
    persist();
    return { ok: true };
  }

  function clearTableAssignment(reservation) {
    reservation.tableId = null;
    reservation.lastModified = formatDateTimeValue(new Date());
    recomputeTables();
    persist();
    return { ok: true };
  }

  function createReservationFromForm(form, { isWalkIn = false } = {}) {
    const name = getFormField(form, "name");
    const phone = getFormField(form, "phone");
    const partySizeRaw = getFormField(form, "partySize");
    if (!name || !phone || !partySizeRaw) {
      return makeTextResponse("缺少必要信息", 400);
    }
    const partySize = Number.parseInt(partySizeRaw, 10);
    if (!Number.isFinite(partySize) || partySize <= 0) {
      return makeTextResponse("就餐人数无效", 400);
    }

    const now = new Date();
    const durationMinutes = defaultDuration;
    let timePoint = now;
    if (!isWalkIn) {
      const requested = getFormField(form, "time");
      const parsed = parseDateTimeInput(requested);
      if (!parsed) {
        return makeTextResponse("时间格式应为 YYYY-MM-DD HH:MM", 400);
      }
      timePoint = parsed;
    }

    const reservationId = isWalkIn
      ? formatIdentifier("W", state.nextWalkInNumber++)
      : formatIdentifier("R", state.nextReservationNumber++);
    const tableId = findAvailableTable(partySize, timePoint, durationMinutes, reservationId);

    const reservation = {
      id: reservationId,
      customer: name,
      phone,
      email: getFormField(form, "email") || "",
      preference: getFormField(form, "preference") || "",
      partySize,
      time: formatDateTimeValue(timePoint),
      endTime: formatDateTimeValue(addMinutes(timePoint, durationMinutes)),
      durationMinutes,
      status: isWalkIn ? "Seated" : "Open",
      notes: getFormField(form, "notes") || "",
      tableId,
      lastModified: formatDateTimeValue(now),
    };

    state.reservations.push(reservation);
    recomputeTables();
    persist();
    return makeJsonResponse({ success: true, id: reservation.id }, 201);
  }

  function handleOrder(form) {
    const reservationId = getFormField(form, "reservationId");
    if (!reservationId) {
      return makeTextResponse("缺少预订编号", 400);
    }
    const reservation = state.reservations.find((item) => item.id === reservationId);
    if (!reservation) {
      return makeTextResponse("未找到对应预订", 404);
    }

    const rawItems = form.get("items") || [];
    const itemsArray = Array.isArray(rawItems) ? rawItems : [rawItems];
    if (itemsArray.length === 0) {
      return makeTextResponse("至少选择一项菜品", 400);
    }

    const parsedItems = [];
    for (const entry of itemsArray) {
      if (typeof entry !== "string") {
        return makeTextResponse("菜品数据无效", 400);
      }
      const [name, quantityRaw] = entry.split("|");
      const quantity = Number.parseInt(quantityRaw, 10);
      if (!name || !Number.isFinite(quantity) || quantity <= 0) {
        return makeTextResponse("菜品格式应为 名称|数量", 400);
      }
      const menuItem = state.menu.find((item) => item.name === name);
      if (!menuItem) {
        return makeTextResponse(`未知菜品：${name}`, 400);
      }
      const lineTotal = Math.round(menuItem.price * quantity * 100) / 100;
      parsedItems.push({
        name: menuItem.name,
        category: menuItem.category,
        price: menuItem.price,
        quantity,
        lineTotal,
      });
    }

    const total = Math.round(parsedItems.reduce((sum, item) => sum + item.lineTotal, 0) * 100) / 100;
    const order = {
      id: `O${state.nextOrderNumber++}`,
      reservationId,
      items: parsedItems,
      total,
    };
    state.orders.push(order);
    persist();
    return makeJsonResponse({ success: true, id: order.id, total: order.total }, 201);
  }

  function updateReservationStatus(id, status) {
    const reservation = state.reservations.find((item) => item.id === id);
    if (!reservation) {
      return makeTextResponse("未找到预订", 404);
    }
    const validStatuses = new Set(["Open", "Seated", "Completed", "Cancelled"]);
    if (!validStatuses.has(status)) {
      return makeTextResponse("状态无效", 400);
    }
    reservation.status = status;
    if (status === "Cancelled") {
      reservation.tableId = null;
    }
    reservation.lastModified = formatDateTimeValue(new Date());
    recomputeTables();
    persist();
    return makeJsonResponse({ success: true });
  }

  function deleteReservationEntry(id) {
    const index = state.reservations.findIndex((item) => item.id === id);
    if (index === -1) {
      return makeTextResponse("未找到预订", 404);
    }
    state.reservations.splice(index, 1);
    removeOrdersByReservation(id);
    recomputeTables();
    persist();
    return makeJsonResponse({ success: true });
  }

  async function handle(url, options = {}) {
    const path = normalizeApiPath(url).split("?")[0];
    const method = (options.method || "GET").toUpperCase();

    if (method === "OPTIONS") {
      return makeJsonResponse({});
    }

    if (method === "GET" && path === "/api/tables") {
      recomputeTables();
      persist();
      return makeJsonResponse(serializeTables());
    }
    if (method === "GET" && path === "/api/reservations") {
      recomputeTables();
      return makeJsonResponse(state.reservations);
    }
    if (method === "GET" && path === "/api/orders") {
      return makeJsonResponse(state.orders);
    }
    if (method === "GET" && path === "/api/menu") {
      return makeJsonResponse(state.menu);
    }
    if (method === "GET" && path === "/api/report") {
      const seatedGuests = state.reservations
        .filter((reservation) => reservation.status === "Seated" || reservation.status === "Completed")
        .reduce((sum, reservation) => sum + reservation.partySize, 0);
      const revenue = state.orders.reduce((sum, order) => sum + order.total, 0);
      const today = formatDateValue(new Date());
      if (state.bookingDate !== today) {
        state.bookingDate = today;
        persist();
      }
      return makeJsonResponse({
        date: state.bookingDate,
        totalReservations: state.reservations.length,
        seatedGuests,
        revenue,
      });
    }
    if (method === "GET" && path === "/api/staff") {
      return makeJsonResponse(state.staff);
    }

    if (method === "POST" && path === "/api/reservations") {
      const form = parseFormBody(options.body);
      return createReservationFromForm(form, { isWalkIn: false });
    }
    if (method === "POST" && path === "/api/walkins") {
      const form = parseFormBody(options.body);
      return createReservationFromForm(form, { isWalkIn: true });
    }
    if (method === "POST" && path === "/api/orders") {
      const form = parseFormBody(options.body);
      return handleOrder(form);
    }

    if (method === "DELETE" && path.startsWith("/api/reservations/")) {
      const id = path.replace("/api/reservations/", "");
      return deleteReservationEntry(id);
    }

    if (method === "POST" && path.endsWith("/status") && path.startsWith("/api/reservations/")) {
      const id = path.slice("/api/reservations/".length, -"/status".length);
      const form = parseFormBody(options.body);
      const status = getFormField(form, "status");
      if (!status) {
        return makeTextResponse("缺少状态", 400);
      }
      return updateReservationStatus(id, status);
    }

    if (method === "POST" && path.endsWith("/table") && path.startsWith("/api/reservations/")) {
      const id = path.slice("/api/reservations/".length, -"/table".length);
      const reservation = getReservationById(id);
      if (!reservation) {
        return makeTextResponse("未找到预订", 404);
      }
      const form = parseFormBody(options.body);
      const mode = (getFormField(form, "mode") || "").toLowerCase();
      let result;
      if (mode === "clear") {
        result = clearTableAssignment(reservation);
      } else if (mode === "auto") {
        result = autoAssignTableForReservation(reservation);
      } else {
        const tableValue = getFormField(form, "tableId");
        if (!tableValue) {
          return makeTextResponse("缺少桌位", 400);
        }
        const parsed = Number.parseInt(tableValue, 10);
        if (!Number.isFinite(parsed) || parsed <= 0) {
          return makeTextResponse("桌位无效", 400);
        }
        result = assignTableManually(reservation, parsed);
      }
      if (!result.ok) {
        return makeTextResponse(result.message, result.status || 400);
      }
      return makeJsonResponse(reservation);
    }

    return makeTextResponse("未知接口", 404);
  }

  recomputeTables();
  persist();

  return { handle };
}

const mockApi = USE_MOCK_API ? createMockApi() : null;

const tablesContainer = document.getElementById("tables");
const reservationTableBody = document.querySelector("#reservationTable tbody");
const reservationForm = document.getElementById("reservationForm");
const walkInForm = document.getElementById("walkInForm");
const reservationFeedback = document.getElementById("reservationFeedback");
const walkInFeedback = document.getElementById("walkInFeedback");
const menuList = document.getElementById("menuList");
const reportContainer = document.getElementById("report");
const orderForm = document.getElementById("orderForm");
const orderReservationSelect = document.getElementById("orderReservation");
const addOrderItemButton = document.getElementById("addOrderItem");
const orderItemsContainer = document.getElementById("orderItems");
const orderFeedback = document.getElementById("orderFeedback");
const ordersTableBody = document.querySelector("#ordersTable tbody");
const staffList = document.getElementById("staffList");

let tablesCache = [];
let reservationsCache = [];
let menuCache = [];

async function apiFetch(url, options = {}) {
  const base = await ensureApiBase();
  return fetch(resolveApi(url, base), options);
}

function resolveApi(path, base = apiBase ?? "") {
  if (/^https?:\/\//.test(path)) {
    return path;
  }
  return `${base}${path}`;
}

function normalizeError(error) {
  if (error instanceof TypeError || error.name === "TypeError") {
    const currentOrigin =
      window.location.origin && window.location.origin !== "null"
        ? window.location.origin
        : FALLBACK_API_BASE;
    const target = apiBase ?? currentOrigin;
    return `无法连接服务器，请确认已启动后端服务（${target}）。`;
  }
  return error.message || "发生未知错误";
}

async function fetchJson(url) {
  const response = await apiFetch(url);
  if (!response.ok) {
    throw new Error(`请求失败: ${response.status}`);
  }
  return response.json();
}

function statusBadge(status) {
  const cls = status.toLowerCase();
  return `<span class="badge ${cls}">${status}</span>`;
}

function renderTables(tables) {
  tablesContainer.innerHTML = "";
  if (!tables || tables.length === 0) {
    tablesContainer.innerHTML = '<p class="muted">暂无桌位数据</p>';
    return;
  }
  tables.forEach((table) => {
    const card = document.createElement("div");
    const statusLabel = table.status || "Free";
    card.className = `table-card ${statusLabel.toLowerCase()}`;
    const assignments = Array.isArray(table.reservations) ? table.reservations : [];
    const assignmentsHtml = assignments.length
      ? assignments
          .map((assignment) => {
            const orders = assignment.orders && assignment.orders.length ? assignment.orders.join("、") : "暂无订单";
            return `
              <div class="table-assignment">
                <div><strong>${assignment.id}</strong>｜${assignment.customer}｜${assignment.partySize}人</div>
                <small>状态：${assignment.status}｜订单：${orders}</small>
              </div>
            `;
          })
          .join("")
      : '<small class="muted">当前无预订/订单</small>';
    card.innerHTML = `
      <strong>桌号 ${table.id}</strong>
      <span>容量：${table.capacity}</span>
      <span>区域：${table.location || "-"}</span>
      <span>状态：${statusLabel}</span>
      <div class="table-assignments">
        ${assignmentsHtml}
      </div>
    `;
    tablesContainer.appendChild(card);
  });
}

function renderReservations(reservations) {
  reservationTableBody.innerHTML = "";
  if (!reservations || reservations.length === 0) {
    const row = document.createElement("tr");
    row.innerHTML = '<td colspan="7">暂无预订</td>';
    reservationTableBody.appendChild(row);
    return;
  }

  const sortedTables = [...tablesCache].sort((a, b) => a.capacity - b.capacity || a.id - b.id);
  reservations.forEach((r) => {
    const row = document.createElement("tr");
    const detailLines = [];
    if (r.phone) {
      detailLines.push(r.phone);
    }
    if (r.email) {
      detailLines.push(r.email);
    }
    if (r.preference) {
      detailLines.push(`偏好：${r.preference}`);
    }
    row.innerHTML = `
      <td>${r.id}</td>
      <td>
        <div>${r.customer}</div>
        ${detailLines.map((line) => `<small>${line}</small>`).join("")}
      </td>
      <td>${r.partySize}</td>
      <td><span title="预计结束：${r.endTime}\n最近更新：${r.lastModified}">${r.time}</span></td>
      <td class="table-cell-control">
        <div class="current-table">${r.tableId ? `桌号 ${r.tableId}` : "未分配"}</div>
        <div class="table-controls"></div>
      </td>
      <td>${statusBadge(r.status)}</td>
      <td class="actions-column"></td>
    `;

    const controlsEl = row.querySelector(".table-controls");
    if (controlsEl) {
      const select = document.createElement("select");
      select.className = "table-select";
      const placeholder = document.createElement("option");
      placeholder.value = "";
      placeholder.textContent = sortedTables.length ? "选择桌位" : "暂无可选桌位";
      select.appendChild(placeholder);
      sortedTables.forEach((table) => {
        const option = document.createElement("option");
        option.value = table.id;
        option.textContent = `桌${table.id}｜${table.capacity}人｜${table.status}`;
        if (table.status === "OutOfService") {
          option.disabled = true;
        }
        select.appendChild(option);
      });
      controlsEl.appendChild(select);

      const actions = document.createElement("div");
      actions.className = "table-actions";

      const manualBtn = document.createElement("button");
      manualBtn.type = "button";
      manualBtn.textContent = "手动分配";
      manualBtn.disabled = sortedTables.length === 0;
      manualBtn.addEventListener("click", async () => {
        if (!select.value) {
          alert("请先选择要分配的桌位。");
          return;
        }
        await assignReservationTable(r.id, select.value);
      });
      actions.appendChild(manualBtn);

      const autoBtn = document.createElement("button");
      autoBtn.type = "button";
      autoBtn.textContent = "自动";
      autoBtn.classList.add("secondary");
      autoBtn.addEventListener("click", async () => {
        await autoAssignReservationTable(r.id);
      });
      actions.appendChild(autoBtn);

      const clearBtn = document.createElement("button");
      clearBtn.type = "button";
      clearBtn.textContent = "释放";
      clearBtn.classList.add("ghost");
      clearBtn.disabled = !r.tableId;
      clearBtn.addEventListener("click", async () => {
        await clearReservationTable(r.id);
      });
      actions.appendChild(clearBtn);

      controlsEl.appendChild(actions);
    }

    const actionsCell = row.querySelector(".actions-column");
    [
      { label: "入座", status: "Seated" },
      { label: "完成", status: "Completed" },
      { label: "取消", status: "Cancelled" },
    ].forEach((action) => {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.textContent = action.label;
      btn.addEventListener("click", async () => {
        await updateReservationStatus(r.id, action.status);
      });
      actionsCell.appendChild(btn);
    });
    const deleteBtn = document.createElement("button");
    deleteBtn.type = "button";
    deleteBtn.textContent = "删除";
    deleteBtn.classList.add("danger");
    deleteBtn.addEventListener("click", async () => {
      await deleteReservation(r.id);
    });
    actionsCell.appendChild(deleteBtn);
    reservationTableBody.appendChild(row);
  });
}

function renderOrders(orders) {
  ordersTableBody.innerHTML = "";
  if (orders.length === 0) {
    const row = document.createElement("tr");
    row.innerHTML = '<td colspan="4">暂无订单</td>';
    ordersTableBody.appendChild(row);
    return;
  }
  orders.forEach((order) => {
    const row = document.createElement("tr");
    const itemsHtml = order.items
      .map(
        (item) =>
          `<div>${item.name} × ${item.quantity}<small> (${item.category} ￥${item.price.toFixed(
            2
          )})</small></div>`
      )
      .join("");
    row.innerHTML = `
      <td>${order.id}</td>
      <td>${order.reservationId}</td>
      <td>${itemsHtml}</td>
      <td>￥${order.total.toFixed(2)}</td>
    `;
    ordersTableBody.appendChild(row);
  });
}

function renderStaff(staff) {
  staffList.innerHTML = "";
  if (staff.length === 0) {
    const li = document.createElement("li");
    li.textContent = "暂无员工信息";
    staffList.appendChild(li);
    return;
  }
  staff.forEach((member) => {
    const li = document.createElement("li");
    li.innerHTML = `<strong>${member.role}</strong>：${member.name}（${member.contact}）`;
    staffList.appendChild(li);
  });
}

function populateReservationOptions() {
  const currentValue = orderReservationSelect.value;
  orderReservationSelect.innerHTML = '<option value="">请选择预订</option>';
  reservationsCache.forEach((reservation) => {
    const option = document.createElement("option");
    option.value = reservation.id;
    option.textContent = `${reservation.id}｜${reservation.customer}｜${reservation.partySize}人｜${reservation.time}`;
    if (reservation.id === currentValue) {
      option.selected = true;
    }
    orderReservationSelect.appendChild(option);
  });
}

function populateMenuOptions(select, selected = "") {
  select.innerHTML = '<option value="">请选择菜品</option>';
  menuCache.forEach((item) => {
    const option = document.createElement("option");
    option.value = item.name;
    option.textContent = `${item.category}｜${item.name}｜￥${item.price.toFixed(2)}`;
    if (item.name === selected) {
      option.selected = true;
    }
    select.appendChild(option);
  });
}

function refreshOrderItemSelects() {
  const selects = orderItemsContainer.querySelectorAll("select");
  selects.forEach((select) => {
    populateMenuOptions(select, select.value);
  });
}

function addOrderItemRow(selected = "", quantity = 1) {
  const row = document.createElement("div");
  row.className = "order-item-row";

  const select = document.createElement("select");
  populateMenuOptions(select, selected);

  const qty = document.createElement("input");
  qty.type = "number";
  qty.min = "1";
  qty.value = quantity.toString();

  const removeBtn = document.createElement("button");
  removeBtn.type = "button";
  removeBtn.textContent = "移除";
  removeBtn.addEventListener("click", () => {
    row.remove();
    if (orderItemsContainer.childElementCount === 0) {
      addOrderItemRow();
    }
  });

  row.appendChild(select);
  row.appendChild(qty);
  row.appendChild(removeBtn);
  orderItemsContainer.appendChild(row);
}

async function updateReservationStatus(id, status) {
  try {
    const params = new URLSearchParams();
    params.set("status", status);
    const response = await apiFetch(`/api/reservations/${id}/status`, {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: params.toString(),
    });
    if (!response.ok) {
      throw new Error("状态更新失败");
    }
    await loadTables();
    await loadReservations();
    await loadReport();
  } catch (error) {
    alert(normalizeError(error));
  }
}

async function deleteReservation(id) {
  if (!window.confirm("删除后将连同该预订的所有订单一起移除，确认继续？")) {
    return;
  }
  try {
    const response = await apiFetch(`/api/reservations/${id}`, {
      method: "DELETE",
    });
    if (!response.ok) {
      const text = await response.text();
      throw new Error(text || "删除失败");
    }
    await loadTables();
    await loadReservations();
    await loadReport();
    await loadOrders();
  } catch (error) {
    alert(normalizeError(error));
  }
}

async function postTableOperation(id, params) {
  const response = await apiFetch(`/api/reservations/${id}/table`, {
    method: "POST",
    headers: {
      "Content-Type": "application/x-www-form-urlencoded",
    },
    body: params.toString(),
  });
  if (!response.ok) {
    const text = await response.text();
    throw new Error(text || "桌位操作失败");
  }
}

async function assignReservationTable(id, tableId) {
  try {
    const params = new URLSearchParams();
    params.set("tableId", tableId);
    await postTableOperation(id, params);
    await loadTables();
    await loadReservations();
  } catch (error) {
    alert(normalizeError(error));
  }
}

async function autoAssignReservationTable(id) {
  try {
    const params = new URLSearchParams();
    params.set("mode", "auto");
    await postTableOperation(id, params);
    await loadTables();
    await loadReservations();
  } catch (error) {
    alert(normalizeError(error));
  }
}

async function clearReservationTable(id) {
  try {
    const params = new URLSearchParams();
    params.set("mode", "clear");
    await postTableOperation(id, params);
    await loadTables();
    await loadReservations();
  } catch (error) {
    alert(normalizeError(error));
  }
}

async function loadTables() {
  try {
    const data = await fetchJson("/api/tables");
    tablesCache = data;
    renderTables(data);
    if (reservationsCache.length > 0) {
      renderReservations(reservationsCache);
    }
  } catch (error) {
    tablesContainer.innerHTML = `<p class="error">${normalizeError(error)}</p>`;
  }
}

async function loadReservations() {
  try {
    const data = await fetchJson("/api/reservations");
    reservationsCache = data;
    renderReservations(data);
    populateReservationOptions();
  } catch (error) {
    reservationTableBody.innerHTML = `<tr><td colspan="7">${normalizeError(error)}</td></tr>`;
  }
}

async function loadOrders() {
  try {
    const data = await fetchJson("/api/orders");
    renderOrders(data);
  } catch (error) {
    ordersTableBody.innerHTML = `<tr><td colspan="4">${normalizeError(error)}</td></tr>`;
  }
}

async function loadMenu() {
  try {
    const data = await fetchJson("/api/menu");
    menuCache = data;
    menuList.innerHTML = "";
    data.forEach((item) => {
      const li = document.createElement("li");
      li.textContent = `${item.category}｜${item.name}｜￥${item.price.toFixed(2)}`;
      menuList.appendChild(li);
    });
    refreshOrderItemSelects();
  } catch (error) {
    menuList.innerHTML = `<li>${normalizeError(error)}</li>`;
  }
}

async function loadReport() {
  try {
    const data = await fetchJson("/api/report");
    reportContainer.innerHTML = `
      <p>日期：${data.date}</p>
      <p>预订数：${data.totalReservations}</p>
      <p>入座人数：${data.seatedGuests}</p>
      <p>营业额：￥${data.revenue.toFixed(2)}</p>
    `;
  } catch (error) {
    reportContainer.textContent = normalizeError(error);
  }
}

async function loadStaff() {
  try {
    const data = await fetchJson("/api/staff");
    renderStaff(data);
  } catch (error) {
    staffList.innerHTML = `<li>${normalizeError(error)}</li>`;
  }
}

async function submitForm(form, url, feedbackEl, successMessage) {
  const formData = new FormData(form);
  const params = new URLSearchParams();
  for (const [key, value] of formData.entries()) {
    params.append(key, value.toString());
  }

  try {
    const response = await apiFetch(url, {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: params.toString(),
    });
    if (!response.ok) {
      const text = await response.text();
      throw new Error(text || "提交失败");
    }
    feedbackEl.textContent = successMessage;
    feedbackEl.style.color = "#10b981";
    form.reset();
    await loadTables();
    await loadReservations();
    await loadReport();
    await loadOrders();
  } catch (error) {
    feedbackEl.textContent = normalizeError(error);
    feedbackEl.style.color = "#ef4444";
  }
}

reservationForm.addEventListener("submit", (event) => {
  event.preventDefault();
  submitForm(reservationForm, "/api/reservations", reservationFeedback, "预订提交成功！");
});

walkInForm.addEventListener("submit", (event) => {
  event.preventDefault();
  submitForm(walkInForm, "/api/walkins", walkInFeedback, "散客登记成功！");
});

addOrderItemButton.addEventListener("click", () => {
  addOrderItemRow();
});

orderForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const reservationId = orderReservationSelect.value;
  if (!reservationId) {
    orderFeedback.textContent = "请选择预订";
    orderFeedback.style.color = "#ef4444";
    return;
  }
  const params = new URLSearchParams();
  params.set("reservationId", reservationId);
  let hasItem = false;
  orderItemsContainer.querySelectorAll(".order-item-row").forEach((row) => {
    const name = row.querySelector("select").value;
    const quantity = parseInt(row.querySelector("input").value, 10);
    if (name && quantity > 0) {
      params.append("items", `${name}|${quantity}`);
      hasItem = true;
    }
  });
  if (!hasItem) {
    orderFeedback.textContent = "请至少选择一项菜品";
    orderFeedback.style.color = "#ef4444";
    return;
  }
  try {
    const response = await apiFetch("/api/orders", {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: params.toString(),
    });
    if (!response.ok) {
      const text = await response.text();
      throw new Error(text || "提交失败");
    }
    const data = await response.json();
    orderFeedback.textContent = `订单提交成功，总计￥${data.total.toFixed(2)}`;
    orderFeedback.style.color = "#10b981";
    orderForm.reset();
    orderItemsContainer.innerHTML = "";
    addOrderItemRow();
    await loadOrders();
    await loadReport();
  } catch (error) {
    orderFeedback.textContent = normalizeError(error);
    orderFeedback.style.color = "#ef4444";
  }
});

document.getElementById("refreshOrders").addEventListener("click", loadOrders);
document.getElementById("refreshTables").addEventListener("click", loadTables);
document.getElementById("refreshReservations").addEventListener("click", loadReservations);
document.getElementById("refreshReport").addEventListener("click", loadReport);
document.getElementById("refreshStaff").addEventListener("click", loadStaff);

Promise.all([loadTables(), loadReservations(), loadMenu(), loadOrders(), loadReport(), loadStaff()])
  .catch(() => {})
  .finally(() => {
    if (orderItemsContainer.childElementCount === 0) {
      addOrderItemRow();
    }
  });
