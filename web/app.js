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

let reservationsCache = [];
let menuCache = [];

const FALLBACK_API_BASE = "http://localhost:8080";
const API_BASE =
  typeof window !== "undefined" && window.__BOOKING_API_BASE__
    ? window.__BOOKING_API_BASE__
    : window.location.protocol === "file:" || window.location.origin === "null"
    ? FALLBACK_API_BASE
    : "";

function resolveApi(path) {
  if (/^https?:\/\//.test(path)) {
    return path;
  }
  return `${API_BASE}${path}`;
}

function normalizeError(error) {
  if (error instanceof TypeError || error.name === "TypeError") {
    const currentOrigin =
      window.location.origin && window.location.origin !== "null"
        ? window.location.origin
        : FALLBACK_API_BASE;
    const target = API_BASE || currentOrigin;
    return `无法连接服务器，请确认已启动后端服务（${target}）。`;
  }
  return error.message || "发生未知错误";
}

async function fetchJson(url) {
  const response = await fetch(resolveApi(url));
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
  tables.forEach((table) => {
    const card = document.createElement("div");
    card.className = `table-card ${table.status.toLowerCase()}`;
    card.innerHTML = `
      <strong>桌号 ${table.id}</strong>
      <span>容量：${table.capacity}</span>
      <span>区域：${table.location || "-"}</span>
      <span>状态：${table.status}</span>
    `;
    tablesContainer.appendChild(card);
  });
}

function renderReservations(reservations) {
  reservationTableBody.innerHTML = "";
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
      <td>${r.tableId ?? "-"}</td>
      <td>${statusBadge(r.status)}</td>
      <td class="actions-column"></td>
    `;
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
    const response = await fetch(resolveApi(`/api/reservations/${id}/status`), {
      method: "POST",
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
      },
      body: params.toString(),
    });
    if (!response.ok) {
      throw new Error("状态更新失败");
    }
    await loadReservations();
  } catch (error) {
    alert(normalizeError(error));
  }
}

async function deleteReservation(id) {
  if (!window.confirm("确认删除该预订并标记为取消？")) {
    return;
  }
  try {
    const response = await fetch(resolveApi(`/api/reservations/${id}`), {
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

async function loadTables() {
  try {
    const data = await fetchJson("/api/tables");
    renderTables(data);
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
    const response = await fetch(resolveApi(url), {
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
    const response = await fetch(resolveApi("/api/orders"), {
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
