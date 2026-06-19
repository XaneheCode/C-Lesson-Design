/* ============================================
   文房 · Multi-condition Query & Stats Dashboard
   ============================================ */

/* ---------- Multi-condition Query ---------- */
function QueryView({ inventory, transactions, purchases, categories = CATEGORIES, loading = false }) {
  const toast = useToast();
  const [dataset, setDataset] = useState("transactions"); // inventory / transactions / purchases
  const [cond, setCond] = useState({
    productId: "",
    name: "",
    category: "",
    maker: "",
    dateFrom: "",
    dateTo: "",
    qtyMin: "",
    qtyMax: "",
    priceMin: "",
    priceMax: "",
  });
  const [remoteRows, setRemoteRows] = useState([]);
  const [queryLoading, setQueryLoading] = useState(loading);

  const reset = () => setCond({
    productId: "", name: "", category: "", maker: "",
    dateFrom: "", dateTo: "", qtyMin: "", qtyMax: "", priceMin: "", priceMax: "",
  });

  useEffect(() => {
    let alive = true;
    const timer = setTimeout(async () => {
      setQueryLoading(true);
      try {
        let rows = [];
        if (dataset === "inventory") {
          const keyword = [cond.productId, cond.name, cond.maker].map(v => v.trim()).find(Boolean) || "";
          rows = await StationeryApi.listProducts({
            category: cond.category,
            keyword,
          });
        } else {
          const exactProductId = /^P\d{3}$/i.test(cond.productId.trim()) ? cond.productId.trim() : "";
          const params = {
            category: cond.category,
            date_from: cond.dateFrom,
            date_to: cond.dateTo,
            product_id: exactProductId,
            sort: "desc",
          };
          rows = dataset === "transactions"
            ? await StationeryApi.listSales(params)
            : await StationeryApi.listPurchases(params);
        }
        if (alive) setRemoteRows(rows);
      } catch (err) {
        if (alive) {
          toast(err?.message || "查询失败", "err");
          setRemoteRows([]);
        }
      } finally {
        if (alive) setQueryLoading(false);
      }
    }, 200);
    return () => {
      alive = false;
      clearTimeout(timer);
    };
  }, [dataset, cond, toast]);

  const source = remoteRows;
  const isLoading = queryLoading && remoteRows.length === 0;

  const result = useMemo(() => {
    return source.filter(r => {
      if (cond.productId) {
        const target = dataset === "inventory" ? r.id : r.productId;
        if (!target.toLowerCase().includes(cond.productId.toLowerCase())) return false;
      }
      if (cond.name && !r.name.toLowerCase().includes(cond.name.toLowerCase())) return false;
      if (cond.category && r.category !== cond.category) return false;
      if (cond.maker) {
        // For inventory, has direct maker. For txn/purchase, look up via inventory
        if (dataset === "inventory") {
          if (!r.maker.toLowerCase().includes(cond.maker.toLowerCase())) return false;
        } else {
          const item = inventory.find(i => i.id === r.productId);
          if (!item || !item.maker.toLowerCase().includes(cond.maker.toLowerCase())) return false;
        }
      }
      if (dataset !== "inventory") {
        if (cond.dateFrom && r.date < cond.dateFrom) return false;
        if (cond.dateTo && r.date > cond.dateTo) return false;
      }
      const qty = dataset === "inventory" ? r.stock : r.qty;
      if (cond.qtyMin !== "" && qty < parseFloat(cond.qtyMin)) return false;
      if (cond.qtyMax !== "" && qty > parseFloat(cond.qtyMax)) return false;
      if (cond.priceMin !== "" && r.price < parseFloat(cond.priceMin)) return false;
      if (cond.priceMax !== "" && r.price > parseFloat(cond.priceMax)) return false;
      return true;
    });
  }, [source, cond, dataset, inventory]);

  // Enrich result for display
  const enriched = result.map(r => {
    if (dataset === "inventory") return r;
    const inv = inventory.find(i => i.id === r.productId);
    return { ...r, maker: inv?.maker || "—", unitPrice: inv?.price || 0, model: inv?.model || "—" };
  });

  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 06 <span className="sep">/</span> ADVANCED QUERY</div>
          <h1 className="page-title"><span className="cn-num">陆</span>多条件查询</h1>
        </div>
        <div className="page-actions">
          <button className="btn btn-sm" onClick={reset}>重置条件</button>
        </div>
      </div>

      <div className="toolbar" style={{ marginBottom: 20 }}>
        <span style={{ fontFamily: "var(--mono)", fontSize: 11, letterSpacing: "0.14em", textTransform: "uppercase", color: "var(--muted)", marginRight: 8 }}>检索范围 ·</span>
        <button className={"btn " + (dataset === "inventory" ? "btn-primary" : "")} onClick={() => setDataset("inventory")}>库存</button>
        <button className={"btn " + (dataset === "transactions" ? "btn-primary" : "")} onClick={() => setDataset("transactions")}>交易</button>
        <button className={"btn " + (dataset === "purchases" ? "btn-primary" : "")} onClick={() => setDataset("purchases")}>进货</button>
      </div>

      <div className="query-form">
        <div className="query-form-grid">
          <div className="field">
            <label>商品编号</label>
            <input className="mono" placeholder="P001" value={cond.productId}
              onChange={e => setCond({ ...cond, productId: e.target.value })} />
          </div>
          <div className="field">
            <label>商品名称</label>
            <input placeholder="如 橡皮" value={cond.name}
              onChange={e => setCond({ ...cond, name: e.target.value })} />
          </div>
          <div className="field">
            <label>类别</label>
            <select value={cond.category} onChange={e => setCond({ ...cond, category: e.target.value })}>
              <option value="">不限</option>
              {categories.map(c => <option key={c} value={c}>{c}</option>)}
            </select>
          </div>
          <div className="field">
            <label>生产厂家</label>
            <input placeholder="如 晨光" value={cond.maker}
              onChange={e => setCond({ ...cond, maker: e.target.value })} />
          </div>
          {dataset !== "inventory" && (
            <>
              <div className="field">
                <label>日期起</label>
                <input type="date" value={cond.dateFrom}
                  onChange={e => setCond({ ...cond, dateFrom: e.target.value })} />
              </div>
              <div className="field">
                <label>日期止</label>
                <input type="date" value={cond.dateTo}
                  onChange={e => setCond({ ...cond, dateTo: e.target.value })} />
              </div>
            </>
          )}
          <div className="field">
            <label>{dataset === "inventory" ? "库存" : "数量"} 最少</label>
            <input type="number" value={cond.qtyMin}
              onChange={e => setCond({ ...cond, qtyMin: e.target.value })} />
          </div>
          <div className="field">
            <label>{dataset === "inventory" ? "库存" : "数量"} 最多</label>
            <input type="number" value={cond.qtyMax}
              onChange={e => setCond({ ...cond, qtyMax: e.target.value })} />
          </div>
          <div className="field">
            <label>价格 ≥ (¥)</label>
            <input type="number" step="0.01" value={cond.priceMin}
              onChange={e => setCond({ ...cond, priceMin: e.target.value })} />
          </div>
          <div className="field">
            <label>价格 ≤ (¥)</label>
            <input type="number" step="0.01" value={cond.priceMax}
              onChange={e => setCond({ ...cond, priceMax: e.target.value })} />
          </div>
        </div>
      </div>

      <div className="toolbar" style={{ justifyContent: "space-between" }}>
        <div style={{ fontFamily: "var(--mono)", fontSize: 11, letterSpacing: "0.14em", textTransform: "uppercase", color: "var(--muted)" }}>
          匹配结果 · {queryLoading ? "加载中" : enriched.length + " 条"}
        </div>
      </div>

      <div className="tbl-wrap">
        <table className="tbl">
          {dataset === "inventory" ? (
            <>
              <thead>
                <tr>
                  <th>编号</th><th>名称</th><th>类别</th><th>厂家</th><th>型号</th>
                  <th style={{ textAlign: "right" }}>库存</th>
                  <th style={{ textAlign: "right" }}>单价</th>
                </tr>
              </thead>
              <tbody>
                {isLoading ? (
                  <TableSkeletonRows rows={6} cols={7} />
                ) : enriched.length === 0 && (
                  <tr><td colSpan="7" className="empty">无匹配记录</td></tr>
                )}
                {!isLoading && enriched.map(r => (
                  <tr key={r.id}>
                    <td className="id-cell">{r.id}</td>
                    <td className="name-cell">{r.name}</td>
                    <td><CategoryTag value={r.category} /></td>
                    <td>{r.maker}</td>
                    <td className="mono" style={{ fontSize: 12 }}>{r.model}</td>
                    <td className="num-cell">{fmtInt(r.stock)}</td>
                    <td className="num-cell">{fmtMoney(r.price)}</td>
                  </tr>
                ))}
              </tbody>
            </>
          ) : (
            <>
              <thead>
                <tr>
                  <th>单号</th><th>日期</th><th>商品编号</th><th>名称</th><th>类别</th><th>厂家</th>
                  <th style={{ textAlign: "right" }}>数量</th>
                  <th style={{ textAlign: "right" }}>{dataset === "transactions" ? "售价" : "单价"}</th>
                  <th style={{ textAlign: "right" }}>库存单价</th>
                  <th style={{ textAlign: "right" }}>小计</th>
                </tr>
              </thead>
              <tbody>
                {isLoading ? (
                  <TableSkeletonRows rows={6} cols={10} />
                ) : enriched.length === 0 && (
                  <tr><td colSpan="10" className="empty">无匹配记录</td></tr>
                )}
                {!isLoading && enriched.map(r => (
                  <tr key={r.id}>
                    <td className="id-cell">{r.id}</td>
                    <td className="mono" style={{ fontSize: 12 }}>{r.date}</td>
                    <td className="id-cell">{r.productId}</td>
                    <td className="name-cell">{r.name}</td>
                    <td><CategoryTag value={r.category} /></td>
                    <td>{r.maker}</td>
                    <td className="num-cell">{fmtInt(r.qty)}</td>
                    <td className="num-cell">{fmtMoney(r.price)}</td>
                    <td className="num-cell muted">{fmtMoney(r.unitPrice)}</td>
                    <td className="num-cell" style={{ fontWeight: 600 }}>{fmtMoney(r.qty * r.price)}</td>
                  </tr>
                ))}
              </tbody>
            </>
          )}
        </table>
      </div>
    </div>
  );
}

/* ---------- Stats Dashboard ---------- */
function LegacyStatsView({ inventory, transactions, purchases }) {
  // 近一周 = TODAY - 6 days inclusive
  const weekAgo = today(-6);
  const eraserItems = inventory.filter(i => i.category === "橡皮");
  const eraserIds = eraserItems.map(i => i.id);

  const lastWeekEraserTx = transactions.filter(t =>
    eraserIds.includes(t.productId) && t.date >= weekAgo
  );
  const eraserRevenue = lastWeekEraserTx.reduce((s, t) => s + t.qty * t.price, 0);
  const eraserUnitsSold = lastWeekEraserTx.reduce((s, t) => s + t.qty, 0);
  const eraserStock = eraserItems.reduce((s, i) => s + i.stock, 0);

  // Daily eraser revenue bars (last 7 days)
  const days = Array.from({ length: 7 }, (_, idx) => today(-6 + idx));
  const dailyRev = days.map(d => {
    const day = lastWeekEraserTx.filter(t => t.date === d);
    return {
      date: d,
      label: d.slice(5),
      qty: day.reduce((s, t) => s + t.qty, 0),
      rev: day.reduce((s, t) => s + t.qty * t.price, 0),
    };
  });
  const maxRev = Math.max(...dailyRev.map(d => d.rev), 1);

  // By eraser SKU
  const byItem = eraserItems.map(it => {
    const sales = lastWeekEraserTx.filter(t => t.productId === it.id);
    return {
      ...it,
      sold: sales.reduce((s, t) => s + t.qty, 0),
      rev: sales.reduce((s, t) => s + t.qty * t.price, 0),
    };
  }).sort((a, b) => b.rev - a.rev);

  // Overall top sellers
  const allTopByCategory = useMemo(() => {
    const buckets = {};
    transactions.forEach(t => {
      buckets[t.category] = (buckets[t.category] || 0) + t.qty * t.price;
    });
    return Object.entries(buckets).map(([k, v]) => ({ name: k, val: v }))
      .sort((a, b) => b.val - a.val);
  }, [transactions]);

  const totalAllRev = transactions.reduce((s, t) => s + t.qty * t.price, 0);

  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 07 <span className="sep">/</span> ANALYTICS</div>
          <h1 className="page-title"><span className="cn-num">柒</span>所有商品销售情况</h1>
        </div>
        <div className="page-actions">
          <span className="mono" style={{ fontSize: 10, color: "var(--muted)", letterSpacing: "0.14em" }}>
            周期 · {weekAgo} ↦ {today(0)}
          </span>
        </div>
      </div>

      <div className="dash">
        {/* Eraser hero card */}
        <div className="dash-card dark full">
          <div className="dash-card-head">
            <div>
              <div className="dash-eyebrow">SPECIAL · 近一周橡皮专题</div>
              <div className="dash-title">橡皮 / Eraser · Past 7 Days</div>
            </div>
            <div style={{ fontFamily: "var(--mono)", fontSize: 10, letterSpacing: "0.14em", textTransform: "uppercase", color: "var(--muted-2)" }}>
              {weekAgo} → {today(0)}
            </div>
          </div>

          <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 32, marginTop: 24 }}>
            <div>
              <div className="dash-eyebrow" style={{ marginBottom: 12 }}>销售总额 · REVENUE</div>
              <div className="big-number">{fmtMoney(eraserRevenue)}</div>
              <div style={{ fontFamily: "var(--mono)", fontSize: 11, color: "var(--muted-2)", marginTop: 8, letterSpacing: "0.06em" }}>
                {eraserUnitsSold} 件售出 · {lastWeekEraserTx.length} 笔交易
              </div>
            </div>
            <div>
              <div className="dash-eyebrow" style={{ marginBottom: 12 }}>当前库存 · STOCK</div>
              <div className="big-number" style={{ color: "var(--paper)" }}>
                {fmtInt(eraserStock)}<span className="small" style={{ color: "var(--muted-2)" }}>件</span>
              </div>
              <div style={{ fontFamily: "var(--mono)", fontSize: 11, color: "var(--muted-2)", marginTop: 8, letterSpacing: "0.06em" }}>
                {eraserItems.length} 个 SKU · 总值 {fmtMoney(eraserItems.reduce((s, i) => s + i.stock * i.price, 0))}
              </div>
            </div>
            <div>
              <div className="dash-eyebrow" style={{ marginBottom: 12 }}>每日销售 · DAILY</div>
              <div className="bar-chart" style={{ height: 140, marginTop: 0 }}>
                {dailyRev.map(d => (
                  <div className="bar-col" key={d.date}>
                    <div className="bar-fill-wrap">
                      <div className="bar-fill" style={{
                        height: Math.max(2, (d.rev / maxRev) * 100) + "%",
                        background: d.date === today(0) ? "var(--vermillion)" : "var(--paper)"
                      }}>
                        {d.rev > 0 && (
                          <div className="bar-value" style={{ color: "var(--paper)" }}>{d.rev.toFixed(0)}</div>
                        )}
                      </div>
                    </div>
                    <div className="bar-label" style={{ color: "var(--muted-2)" }}>{d.label}</div>
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* Eraser breakdown */}
        <div className="dash-card">
          <div className="dash-card-head">
            <div>
              <div className="dash-eyebrow">BY SKU</div>
              <div className="dash-title">橡皮各品销售明细</div>
            </div>
          </div>
          <div className="top-list">
            {byItem.map((it, idx) => (
              <div className="top-row" key={it.id}>
                <div className="top-rank">{idx + 1}</div>
                <div>
                  <div className="top-name">{it.name}</div>
                  <div className="top-meta">
                    {it.id} · {it.maker} · 当前库存 {fmtInt(it.stock)} 件
                  </div>
                </div>
                <div>
                  <div className="top-val">{fmtMoney(it.rev)}</div>
                  <div className="top-meta" style={{ textAlign: "right" }}>{it.sold} 件</div>
                </div>
              </div>
            ))}
          </div>
        </div>

        {/* Overall revenue by category */}
        <div className="dash-card">
          <div className="dash-card-head">
            <div>
              <div className="dash-eyebrow">OVERALL</div>
              <div className="dash-title">分类销售占比</div>
            </div>
          </div>
          <div className="top-list">
            {allTopByCategory.map((c, idx) => {
              const pct = totalAllRev ? (c.val / totalAllRev) * 100 : 0;
              return (
                <div key={c.name} style={{ marginBottom: 12 }}>
                  <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 6, fontSize: 13 }}>
                    <span><CategoryTag value={c.name} /></span>
                    <span className="mono">{fmtMoney(c.val)} · {pct.toFixed(1)}%</span>
                  </div>
                  <div style={{ height: 8, background: "var(--paper-2)", border: "1px solid var(--line)" }}>
                    <div style={{
                      height: "100%",
                      width: pct + "%",
                      background: c.name === "橡皮" ? "var(--vermillion)" : "var(--ink)",
                      transition: "width .5s ease"
                    }} />
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        {/* All categories summary */}
        <div className="dash-card full">
          <div className="dash-card-head">
            <div>
              <div className="dash-eyebrow">LEDGER SNAPSHOT · 全店概览</div>
              <div className="dash-title">各类目库存与本期销售</div>
            </div>
          </div>
          <div className="tbl-wrap" style={{ marginTop: 12 }}>
            <table className="tbl">
              <thead>
                <tr>
                  <th>类别</th>
                  <th style={{ textAlign: "right" }}>SKU 数</th>
                  <th style={{ textAlign: "right" }}>库存</th>
                  <th style={{ textAlign: "right" }}>库存总值</th>
                  <th style={{ textAlign: "right" }}>累计销售</th>
                  <th style={{ textAlign: "right" }}>近一周销售</th>
                </tr>
              </thead>
              <tbody>
                {CATEGORIES.map(c => {
                  const items = inventory.filter(i => i.category === c);
                  const txAll = transactions.filter(t => t.category === c);
                  const txWeek = txAll.filter(t => t.date >= weekAgo);
                  return (
                    <tr key={c}>
                      <td><CategoryTag value={c} /></td>
                      <td className="num-cell">{items.length}</td>
                      <td className="num-cell">{fmtInt(items.reduce((s, i) => s + i.stock, 0))}</td>
                      <td className="num-cell">{fmtMoney(items.reduce((s, i) => s + i.stock * i.price, 0))}</td>
                      <td className="num-cell">{fmtMoney(txAll.reduce((s, t) => s + t.qty * t.price, 0))}</td>
                      <td className="num-cell" style={ c === "橡皮" ? { color: "var(--vermillion)", fontWeight: 600 } : null }>
                        {fmtMoney(txWeek.reduce((s, t) => s + t.qty * t.price, 0))}
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
}

function StatsView({ inventory, transactions, purchases, categories = CATEGORIES, loading = false }) {
  const toast = useToast();
  const viewRef = useRef(null);
  const animatedRef = useRef(false);
  const [stats, setStats] = useState({
    dashboard: null,
    weekly: null,
    ranking: [],
    inventoryProducts: [],
    lowStock: { alerts: [], total: 0 },
  });
  const [statsLoading, setStatsLoading] = useState(true);
  const [statsError, setStatsError] = useState("");

  const loadStats = useCallback(async () => {
    setStatsLoading(true);
    setStatsError("");
    try {
      const [dashboard, weekly, ranking, inventoryReport, lowStock] = await Promise.all([
        StationeryApi.getDashboardStats(),
        StationeryApi.getWeeklyStats(),
        StationeryApi.getRanking(),
        StationeryApi.getInventoryStats(),
        StationeryApi.getLowStock(),
      ]);
      setStats({
        dashboard: dashboard || {},
        weekly: weekly || {},
        ranking: ranking?.ranking || [],
        inventoryProducts: inventoryReport?.products || [],
        lowStock: lowStock || { alerts: [], total: 0 },
      });
    } catch (err) {
      const msg = err?.message || "统计数据加载失败";
      setStatsError(msg);
      toast(msg, "err");
    } finally {
      setStatsLoading(false);
    }
  }, [toast]);

  useEffect(() => {
    loadStats();
  }, [loadStats]);

  const n = (v) => {
    const value = Number(v);
    return Number.isFinite(value) ? value : 0;
  };

  const dashboard = stats.dashboard || {};
  const weekly = stats.weekly || {};
  const periodStart = weekly.date_from || today(-6);
  const periodEnd = weekly.date_to || today(0);
  const weeklyDaily = weekly.daily || [];
  const maxRev = Math.max(...weeklyDaily.map(d => n(d.amount)), 1);
  const reportProducts = stats.inventoryProducts || [];
  const rankingRows = (stats.ranking || []).slice(0, 10);
  const maxRanking = Math.max(...rankingRows.map(row => n(row.total_amount)), 1);
  const detailRows = reportProducts
    .map(p => ({
      ...p,
      stock: n(p.stock),
      sold_qty: n(p.sold_qty),
      sold_amount: n(p.sold_amount),
    }))
    .sort((a, b) => b.sold_amount - a.sold_amount);
  const statCategories = useMemo(() => {
    const all = [...categories, ...reportProducts.map(p => p.category).filter(Boolean)];
    return [...new Set(all)].filter(Boolean);
  }, [categories, reportProducts]);

  const categoryRows = useMemo(() => statCategories.map(c => {
    const items = reportProducts.filter(p => p.category === c);
    return {
      name: c,
      sku: items.length,
      stock: items.reduce((s, p) => s + n(p.stock), 0),
      value: items.reduce((s, p) => s + n(p.inventory_value), 0),
      sold: items.reduce((s, p) => s + n(p.sold_amount), 0),
      bought: items.reduce((s, p) => s + n(p.bought_cost), 0),
    };
  }).filter(row => row.sku > 0), [statCategories, reportProducts]);
  const totalCategorySales = Math.max(categoryRows.reduce((s, row) => s + row.sold, 0), 1);
  const donutColors = ["#C8341D", "#2C5530", "#B08545", "#6B6358", "#9B2614", "#3B5BA0"];
  const donutCircumference = 2 * Math.PI * 42;
  let donutOffset = 0;
  const donutRows = categoryRows.map((row, idx) => {
    const share = row.sold / totalCategorySales;
    const segment = {
      ...row,
      color: donutColors[idx % donutColors.length],
      share,
      dash: share * donutCircumference,
      offset: donutOffset,
    };
    donutOffset += segment.dash;
    return segment;
  });

  const firstLoad = statsLoading && !stats.dashboard;

  useEffect(() => {
    if (firstLoad || animatedRef.current || !viewRef.current || !window.gsap) return undefined;

    const gsap = window.gsap;
    animatedRef.current = true;
    let finishTimer;
    const ctx = gsap.context(() => {
      const timeline = gsap.timeline({ defaults: { ease: "power3.out" } });
      timeline
        .from(".sales-stat-cell", { y: 14, opacity: 0, duration: 0.45, stagger: 0.06 })
        .from(".weekly-sales-bar", { scaleY: 0, transformOrigin: "bottom", duration: 0.65, stagger: 0.06 }, "-=0.22")
        .from(".sales-donut-segment", { strokeDasharray: `0 ${donutCircumference}`, duration: 0.75, stagger: 0.08 }, "-=0.5")
        .from(".ranking-bar-fill", { scaleX: 0, transformOrigin: "left", duration: 0.55, stagger: 0.045 }, "-=0.52")
        .from(".sales-detail-table", { y: 14, opacity: 0, duration: 0.45 }, "-=0.25");

      const countEls = viewRef.current.querySelectorAll("[data-count-value]");
      countEls.forEach(el => {
        const target = n(el.dataset.countValue);
        const kind = el.dataset.countKind;
        const counter = { value: 0 };
        gsap.to(counter, {
          value: target,
          duration: 0.8,
          ease: "power2.out",
          onUpdate: () => {
            el.textContent = kind === "money" ? fmtMoney(counter.value) : fmtInt(Math.round(counter.value));
          },
        });
      });

      finishTimer = window.setTimeout(() => {
        timeline.progress(1);
        countEls.forEach(el => {
          const target = n(el.dataset.countValue);
          el.textContent = el.dataset.countKind === "money" ? fmtMoney(target) : fmtInt(target);
        });
      }, 3200);

      timeline.eventCallback("onComplete", () => window.clearTimeout(finishTimer));
    }, viewRef);

    return () => {
      window.clearTimeout(finishTimer);
      ctx.revert();
    };
  }, [firstLoad]);

  return (
    <div ref={viewRef}>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 07 <span className="sep">/</span> ANALYTICS</div>
          <h1 className="page-title"><span className="cn-num">柒</span>所有商品销售情况</h1>
        </div>
        <div className="page-actions">
          <span className="mono" style={{ fontSize: 10, color: "var(--muted)", letterSpacing: "0.14em" }}>
            周期 · {periodStart} ↦ {periodEnd}
          </span>
          <button className="btn btn-sm" onClick={loadStats} disabled={statsLoading}>
            {statsLoading ? "加载中…" : "刷新统计"}
          </button>
        </div>
      </div>

      {statsError && (
        <div className="api-error inline">
          <div>
            <div className="api-error-title">统计加载失败</div>
            <div className="api-error-msg">{statsError}</div>
          </div>
          <button className="btn btn-sm" onClick={loadStats}>重新加载</button>
        </div>
      )}

      {firstLoad ? <StatsSkeleton /> : (
        <div className="stats-strip">
          <div className="stat-cell sales-stat-cell">
            <div className="stat-label">商品种类</div>
            <div className="stat-value"><span data-count-value={n(dashboard.product_count)}>{fmtInt(n(dashboard.product_count))}</span><span className="unit">种</span></div>
            <div className="stat-foot">total products</div>
          </div>
          <div className="stat-cell sales-stat-cell">
            <div className="stat-label">历史累计销量</div>
            <div className="stat-value"><span data-count-value={n(dashboard.total_sale_qty)}>{fmtInt(n(dashboard.total_sale_qty))}</span><span className="unit">件</span></div>
            <div className="stat-foot">units sold</div>
          </div>
          <div className="stat-cell sales-stat-cell">
            <div className="stat-label">历史累计销售额</div>
            <div className="stat-value vermillion"><span data-count-value={n(dashboard.total_sales)} data-count-kind="money">{fmtMoney(n(dashboard.total_sales))}</span></div>
            <div className="stat-foot up">gross sales</div>
          </div>
          <div className="stat-cell sales-stat-cell">
            <div className="stat-label">低库存预警</div>
            <div className="stat-value"><span data-count-value={n(stats.lowStock?.total ?? dashboard.low_stock_count)}>{fmtInt(n(stats.lowStock?.total ?? dashboard.low_stock_count))}</span><span className="unit">项</span></div>
            <div className="stat-foot down">stock &lt; 20</div>
          </div>
        </div>
      )}

      <div className="dash">
        {firstLoad ? (
          <>
            <div className="dash-card dark full"><StatsSkeleton /></div>
            <div className="dash-card">
              <div className="top-list">
                {Array.from({ length: 4 }, (_, idx) => (
                  <div className="top-row" key={idx}>
                    <span className="sk sk-line" />
                    <span className="sk sk-line" />
                  </div>
                ))}
              </div>
            </div>
            <div className="dash-card">
              <div className="top-list">
                {Array.from({ length: 4 }, (_, idx) => (
                  <div className="top-row" key={idx}>
                    <span className="sk sk-line" />
                    <span className="sk sk-line" />
                  </div>
                ))}
              </div>
            </div>
          </>
        ) : (
          <>
            <div className="dash-card dark sales-trend-card">
              <div className="dash-card-head">
                <div>
                  <div className="dash-eyebrow">SALES TREND · 全店实时汇总</div>
                  <div className="dash-title">近 7 天全店销售趋势</div>
                </div>
                <div style={{ fontFamily: "var(--mono)", fontSize: 10, letterSpacing: "0.14em", textTransform: "uppercase", color: "var(--muted-2)" }}>
                  {periodStart} → {periodEnd}
                </div>
              </div>

              <div className="trend-summary">
                <div>
                  <div className="dash-eyebrow">近 7 天销售额 · REVENUE</div>
                  <div className="trend-total">{fmtMoney(n(weekly.total_amount))}</div>
                  <div className="trend-meta">
                    {fmtInt(n(weekly.total_qty))} 件售出 · 本月 {fmtMoney(n(dashboard.month_sales))}
                  </div>
                </div>
                <div className="bar-chart sales-week-chart">
                  {weeklyDaily.map(d => (
                    <div className="bar-col" key={d.date}>
                      <div className="bar-fill-wrap">
                        <div className="bar-fill weekly-sales-bar" style={{
                          height: Math.max(2, (n(d.amount) / maxRev) * 100) + "%",
                          background: d.date === periodEnd ? "var(--vermillion)" : "var(--paper)"
                        }}>
                          {n(d.amount) > 0 && (
                            <div className="bar-value" style={{ color: "var(--paper)" }}>{n(d.amount).toFixed(0)}</div>
                          )}
                        </div>
                      </div>
                      <div className="bar-label" style={{ color: "var(--muted-2)" }}>{(d.date || "").slice(5)}</div>
                    </div>
                  ))}
                </div>
              </div>
            </div>

            <div className="dash-card category-share-card">
              <div className="dash-card-head">
                <div>
                  <div className="dash-eyebrow">CATEGORY SHARE</div>
                  <div className="dash-title">品类销售额占比</div>
                </div>
              </div>
              <div className="category-share-body">
                <svg className="sales-donut" viewBox="0 0 112 112" role="img" aria-label="品类销售额占比环形图">
                  <circle className="sales-donut-track" cx="56" cy="56" r="42" />
                  {donutRows.map(row => (
                    <circle
                      className="sales-donut-segment"
                      key={row.name}
                      cx="56" cy="56" r="42"
                      stroke={row.color}
                      strokeDasharray={`${row.dash} ${donutCircumference}`}
                      strokeDashoffset={-row.offset}
                    />
                  ))}
                </svg>
                <div className="category-legend">
                  {donutRows.map(row => (
                    <div className="category-legend-row" key={row.name}>
                      <span className="category-dot" style={{ background: row.color }} />
                      <span>{row.name}</span>
                      <strong>{(row.share * 100).toFixed(1)}%</strong>
                    </div>
                  ))}
                </div>
              </div>
            </div>

            <div className="dash-card full ranking-chart-card">
              <div className="dash-card-head">
                <div>
                  <div className="dash-eyebrow">RANKING · HISTORICAL SALES</div>
                  <div className="dash-title">历史累计商品销售额 Top 10</div>
                </div>
              </div>
              <div className="ranking-chart">
                {rankingRows.length === 0 && <div className="empty">暂无排行数据</div>}
                {rankingRows.map(row => (
                  <div className="ranking-bar-row" key={row.product_id}>
                    <div className="top-rank">{row.rank}</div>
                    <div className="ranking-bar-main">
                      <div className="ranking-bar-head">
                        <span className="top-name">{row.name}</span>
                        <span className="top-val">{fmtMoney(n(row.total_amount))}</span>
                      </div>
                      <div className="ranking-bar-track">
                        <div className="ranking-bar-fill" style={{ width: (n(row.total_amount) / maxRanking * 100) + "%" }} />
                      </div>
                      <div className="top-meta">{row.product_id} · {row.category} · {fmtInt(n(row.total_qty))} 件 · 库存 {fmtInt(n(row.current_stock))}</div>
                    </div>
                  </div>
                ))}
              </div>
            </div>

            <div className="dash-card full">
              <div className="dash-card-head">
                <div>
                  <div className="dash-eyebrow">LEDGER SNAPSHOT · ALL PRODUCTS</div>
                  <div className="dash-title">所有商品销售明细</div>
                </div>
              </div>
              <div className="tbl-wrap" style={{ marginTop: 12 }}>
                <table className="tbl sales-detail-table">
                  <thead>
                    <tr>
                      <th>商品编号</th>
                      <th>商品名称</th>
                      <th>类别</th>
                      <th style={{ textAlign: "right" }}>累计销量</th>
                      <th style={{ textAlign: "right" }}>累计销售额</th>
                      <th style={{ textAlign: "right" }}>当前库存</th>
                    </tr>
                  </thead>
                  <tbody>
                    {detailRows.length === 0 && (
                      <tr><td colSpan="6" className="empty">暂无商品销售数据</td></tr>
                    )}
                    {detailRows.map(row => (
                      <tr key={row.id}>
                        <td className="id-cell">{row.id}</td>
                        <td className="name-cell">{row.name}</td>
                        <td><CategoryTag value={row.category} /></td>
                        <td className="num-cell">{fmtInt(row.sold_qty)}</td>
                        <td className="num-cell">{fmtMoney(row.sold_amount)}</td>
                        <td className="num-cell">{fmtInt(row.stock)}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>
          </>
        )}
      </div>
    </div>
  );
}

/* ---------- Help / About ---------- */
function HelpView() {
  const sections = [
    { num: "壹", title: "库存管理", desc: "查看、新增、修改、删除文具商品库存。支持按编号、名称、厂家、型号、类别筛选，所有字段可排序。低于 30 件的库存自动标红预警。" },
    { num: "贰", title: "交易记录", desc: "查看所有销售流水。支持按交易日期升序/降序排序，多字段排序与筛选。仅支持修改与删除（新建请使用「新建销售」）。" },
    { num: "叁", title: "进货记录", desc: "查看所有进货单据。多字段筛选与排序，支持修改、删除（新建请使用「录入进货」）。" },
    { num: "肆", title: "新建销售", desc: "选择商品 → 输入数量、议价售价 → 确认。系统自动扣减库存、生成交易号、写入账册，发票样式实时预览。" },
    { num: "伍", title: "录入进货", desc: "两种模式：① 已有商品补货 ② 新品入库（创建库存记录）。自动增加库存、生成进货单号。可即时查看进货毛利率。" },
    { num: "陆", title: "多条件查询", desc: "在库存 / 交易 / 进货三个数据集上自由组合：商品编号、名称、类别、厂家、日期区间、数量区间、价格区间。支持您题目示例 —— 「查询某一天所有销售商品的名称、编号、厂家、售价和单价」。" },
    { num: "柒", title: "经营统计", desc: "展示所有商品销售情况：近 7 天全店销售趋势、品类销售额占比、历史累计商品销售额 Top 10，以及所有商品销售明细。" },
  ];
  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 08 <span className="sep">/</span> DOCUMENTATION</div>
          <h1 className="page-title"><span className="cn-num">捌</span>使用说明</h1>
        </div>
      </div>

      <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 1, background: "var(--line)", border: "1px solid var(--line)" }}>
        {sections.map(s => (
          <div key={s.num} style={{ background: "var(--paper-3)", padding: 28 }}>
            <div style={{ display: "flex", alignItems: "baseline", gap: 16, marginBottom: 12 }}>
              <span className="serif vermillion" style={{ fontSize: 36, fontWeight: 700, lineHeight: 1 }}>{s.num}</span>
              <span className="serif" style={{ fontSize: 22, fontWeight: 600 }}>{s.title}</span>
            </div>
            <p style={{ fontSize: 14, lineHeight: 1.75, color: "var(--ink-2)", textWrap: "pretty" }}>{s.desc}</p>
          </div>
        ))}
      </div>

      <div style={{ marginTop: 32, padding: 28, background: "var(--ink)", color: "var(--paper)" }}>
        <div className="dash-eyebrow" style={{ color: "var(--muted-2)" }}>设计原则 · DESIGN PRINCIPLES</div>
        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr 1fr", gap: 32, marginTop: 16 }}>
          <div>
            <div className="serif" style={{ fontSize: 18, fontWeight: 600, marginBottom: 6 }}>编辑设计语言</div>
            <p style={{ fontSize: 13, lineHeight: 1.7, color: "var(--muted-2)" }}>以杂志版面美学组织数据。巨字号衬线标题、单色朱砂强调、慷慨留白，让数字与文字都有呼吸。</p>
          </div>
          <div>
            <div className="serif" style={{ fontSize: 18, fontWeight: 600, marginBottom: 6 }}>所有操作即时可见</div>
            <p style={{ fontSize: 13, lineHeight: 1.7, color: "var(--muted-2)" }}>销售/进货页右侧"账册卡"实时预览，所变即所得；列表页顶部统计条提供四个关键指标。</p>
          </div>
          <div>
            <div className="serif" style={{ fontSize: 18, fontWeight: 600, marginBottom: 6 }}>数据后端同步</div>
            <p style={{ fontSize: 13, lineHeight: 1.7, color: "var(--muted-2)" }}>业务数据由后端接口读取与写入。点击侧栏 ⟲ 可重新加载最新数据。</p>
          </div>
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { QueryView, StatsView, HelpView });
