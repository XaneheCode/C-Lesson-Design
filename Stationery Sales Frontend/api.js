(function () {
  const BASE_URL = "http://localhost:8080/api";

  const isBlank = (v) => v === undefined || v === null || v === "";
  const toNumber = (v) => {
    const n = Number(v);
    return Number.isFinite(n) ? n : 0;
  };

  function compact(obj) {
    return Object.fromEntries(
      Object.entries(obj).filter(([, value]) => value !== undefined)
    );
  }

  function buildUrl(path, params) {
    const url = new URL(BASE_URL + path);
    if (params) {
      Object.entries(params).forEach(([key, value]) => {
        if (!isBlank(value)) url.searchParams.set(key, value);
      });
    }
    return url.toString();
  }

  async function request(path, options = {}) {
    const { params, body, headers, timeoutMs = 8000, ...rest } = options;
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), timeoutMs);
    let response;
    try {
      response = await fetch(buildUrl(path, params), {
        ...rest,
        signal: rest.signal || controller.signal,
        headers: {
          "Content-Type": "application/json",
          ...(headers || {}),
        },
        body: body === undefined
          ? undefined
          : (typeof body === "string" ? body : JSON.stringify(body)),
      });
    } catch (err) {
      if (err?.name === "AbortError") throw new Error("请求后端超时，请确认服务已启动");
      throw new Error("请求后端失败：" + (err?.message || "网络连接异常"));
    } finally {
      clearTimeout(timer);
    }

    const raw = await response.text();
    let data = null;
    if (raw) {
      try {
        data = JSON.parse(raw);
      } catch {
        throw new Error("无法解析服务器响应");
      }
    }

    if (!response.ok || data?.error) {
      throw new Error(data?.error || data?.message || `请求失败 (${response.status})`);
    }
    return data;
  }

  function normalizeProduct(row = {}) {
    return {
      id: row.id || "",
      name: row.name || "",
      category: row.category || "",
      maker: row.manufacturer ?? row.maker ?? "",
      model: row.model || "",
      stock: toNumber(row.stock),
      price: toNumber(row.price),
    };
  }

  function denormalizeProduct(row = {}) {
    return compact({
      name: row.name,
      category: row.category,
      manufacturer: row.maker ?? row.manufacturer,
      model: row.model,
      stock: row.stock === undefined ? undefined : toNumber(row.stock),
      price: row.price === undefined ? undefined : toNumber(row.price),
    });
  }

  function normalizeRecord(row = {}) {
    const qty = toNumber(row.quantity ?? row.qty);
    const price = toNumber(row.price);
    return {
      id: row.id || "",
      productId: row.product_id ?? row.productId ?? "",
      name: row.name || "",
      category: row.category || "",
      date: row.date || "",
      qty,
      price,
      total: row.total === undefined ? qty * price : toNumber(row.total),
    };
  }

  function denormalizeRecord(row = {}, options = {}) {
    const includeProduct = options.includeProduct !== false;
    return compact({
      product_id: includeProduct ? (row.productId ?? row.product_id) : undefined,
      date: row.date,
      quantity: row.qty === undefined ? undefined : toNumber(row.qty),
      price: row.price === undefined ? undefined : toNumber(row.price),
    });
  }

  async function listProducts(params) {
    const data = await request("/products", { params });
    return (data?.data || []).map(normalizeProduct);
  }

  async function listSales(params) {
    const data = await request("/sales", { params });
    return (data?.data || []).map(normalizeRecord);
  }

  async function listPurchases(params) {
    const data = await request("/purchases", { params });
    return (data?.data || []).map(normalizeRecord);
  }

  const Api = {
    BASE_URL,
    request,
    normalizeProduct,
    denormalizeProduct,
    normalizeRecord,
    denormalizeRecord,
    listProducts,
    listSales,
    listPurchases,
    createProduct: (product) => request("/products", {
      method: "POST",
      body: denormalizeProduct(product),
    }),
    updateProduct: (id, product) => request(`/products/${encodeURIComponent(id)}`, {
      method: "PUT",
      body: denormalizeProduct(product),
    }),
    deleteProduct: (id) => request(`/products/${encodeURIComponent(id)}`, {
      method: "DELETE",
    }),
    createSale: (record) => request("/sales", {
      method: "POST",
      body: denormalizeRecord(record),
    }),
    updateSale: (id, record) => request(`/sales/${encodeURIComponent(id)}`, {
      method: "PUT",
      body: denormalizeRecord(record, { includeProduct: false }),
    }),
    deleteSale: (id) => request(`/sales/${encodeURIComponent(id)}`, {
      method: "DELETE",
    }),
    createPurchase: (record) => request("/purchases", {
      method: "POST",
      body: denormalizeRecord(record),
    }),
    updatePurchase: (id, record) => request(`/purchases/${encodeURIComponent(id)}`, {
      method: "PUT",
      body: denormalizeRecord(record, { includeProduct: false }),
    }),
    deletePurchase: (id) => request(`/purchases/${encodeURIComponent(id)}`, {
      method: "DELETE",
    }),
    getDashboardStats: () => request("/stats/dashboard"),
    getRanking: (params) => request("/stats/ranking", { params }),
    getLowStock: () => request("/stats/low-stock"),
    getWeeklyStats: (category) => request("/stats/weekly", { params: { category } }),
    getInventoryStats: () => request("/stats/inventory"),
  };

  window.StationeryApi = Api;
})();
