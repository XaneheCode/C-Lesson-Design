/* ============================================
   文房 · Sell & Restock action pages
   ============================================ */

/* ---------- 新建销售 ---------- */
function SellView({ inventory, refreshProducts, refreshTransactions, goTo }) {
  const toast = useToast();
  const [productId, setProductId] = useState("");
  const [qty, setQty] = useState(1);
  const [price, setPrice] = useState(0);
  const [date, setDate] = useState(today(0));
  const [pending, setPending] = useState(false);

  const product = inventory.find(i => i.id === productId);

  useEffect(() => {
    if (product) setPrice(product.price);
  }, [productId]);

  const subtotal = (qty || 0) * (price || 0);
  const canSubmit = !pending && product && qty > 0 && price >= 0 && qty <= product.stock;

  const submit = async () => {
    if (!product) { toast("请选择商品", "err"); return; }
    if (qty <= 0) { toast("数量必须大于 0", "err"); return; }
    if (qty > product.stock) { toast("库存不足，仅剩 " + product.stock, "err"); return; }

    setPending(true);
    try {
      await StationeryApi.createSale({
        productId: product.id,
        date,
        qty,
        price,
      });
      await Promise.all([refreshProducts(), refreshTransactions()]);
      toast(`已售出 ${qty} 件 · ${product.name} · ${fmtMoney(subtotal)}`);
      setProductId("");
      setQty(1);
      setPrice(0);
    } catch (err) {
      toast(err?.message || "销售提交失败", "err");
    } finally {
      setPending(false);
    }
  };

  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 04 <span className="sep">/</span> NEW SALE</div>
          <h1 className="page-title"><span className="cn-num">肆</span>新建销售</h1>
        </div>
        <div className="page-actions">
          <button className="btn" onClick={() => goTo("transactions")}>查看交易记录 →</button>
        </div>
      </div>

      <div className="action-page">
        <div className="action-form">
          <div className="form-grid">
            <div className="field full">
              <label>选择商品 · Select Product</label>
              <select value={productId} onChange={e => setProductId(e.target.value)}>
                <option value="">— 从库存中选择 —</option>
                {inventory.map(i => (
                  <option key={i.id} value={i.id} disabled={i.stock <= 0}>
                    {i.id} · {i.name} {i.stock <= 0 ? "(缺货)" : `· 库存 ${i.stock}`}
                  </option>
                ))}
              </select>
            </div>
            <div className="field">
              <label>交易日期 · Date</label>
              <input type="date" value={date} onChange={e => setDate(e.target.value)} />
            </div>
            <div className="field">
              <label>类别 · Category</label>
              <input value={product?.category || "—"} disabled />
            </div>
            <div className="field">
              <label>数量 · Quantity</label>
              <input type="number" min="1" max={product?.stock || 1}
                value={qty} onChange={e => setQty(parseInt(e.target.value || "0", 10))} />
              {product && <span className="help">最多可售 {product.stock} 件</span>}
            </div>
            <div className="field">
              <label>售价 · Unit Price (¥)</label>
              <input type="number" min="0" step="0.01"
                value={price} onChange={e => setPrice(parseFloat(e.target.value || "0"))} />
              {product && (
                <span className="help">
                  库存价 {fmtMoney(product.price)}
                  {price !== product.price && price > 0 && (
                    <span className="vermillion"> · 议价 {price > product.price ? "+" : ""}{(((price - product.price) / product.price) * 100).toFixed(1)}%</span>
                  )}
                </span>
              )}
            </div>
            <div className="action-tip full">
              销售操作将自动扣减库存 · 生成新交易号 · 立即写入账册
            </div>
          </div>
        </div>

        <div className="action-summary">
          <div>
            <div className="summary-key">销售明细 · INVOICE</div>
            <div style={{ marginTop: 16, fontFamily: "var(--serif)", fontSize: 22, fontWeight: 600 }}>
              {product ? product.name : "尚未选择商品"}
            </div>
            <div style={{ marginTop: 4, fontFamily: "var(--mono)", fontSize: 11, color: "var(--muted-2)" }}>
              {product ? `${product.id} · ${product.maker} · ${product.model}` : "— · — · —"}
            </div>
          </div>

          <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
            <div className="summary-row">
              <span className="summary-key">日期</span>
              <span className="summary-val">{date}</span>
            </div>
            <div className="summary-row">
              <span className="summary-key">数量</span>
              <span className="summary-val">{qty} 件</span>
            </div>
            <div className="summary-row">
              <span className="summary-key">单价</span>
              <span className="summary-val">{fmtMoney(price)}</span>
            </div>
            <div className="summary-row">
              <span className="summary-key">售后库存</span>
              <span className="summary-val">
                {product ? Math.max(0, product.stock - qty) + " 件" : "—"}
              </span>
            </div>
          </div>

          <div>
            <div className="summary-key" style={{ marginBottom: 8 }}>应收总额 · TOTAL</div>
            <div className="summary-total">{fmtMoney(subtotal)}</div>
          </div>

          <button
            className="btn btn-primary"
            style={{ marginTop: "auto", padding: "16px 24px", fontSize: 12 }}
            disabled={!canSubmit}
            onClick={submit}
          >
            {pending ? "处理中…" : "▸ 确认销售"}
          </button>
        </div>
      </div>
    </div>
  );
}

/* ---------- 录入进货 ---------- */
function RestockView({ inventory, refreshProducts, refreshPurchases, categories = CATEGORIES, goTo }) {
  const toast = useToast();
  const [mode, setMode] = useState("existing"); // existing / new
  const [productId, setProductId] = useState("");
  const [qty, setQty] = useState(1);
  const [price, setPrice] = useState(0);
  const [date, setDate] = useState(today(0));
  const [pending, setPending] = useState(false);
  const [newItem, setNewItem] = useState({
    id: "",
    name: "", category: categories[0] || "", maker: "", model: "", price: 0,
  });

  const product = inventory.find(i => i.id === productId);

  const submit = async () => {
    if (mode === "existing") {
      if (!product) { toast("请选择商品", "err"); return; }
      if (qty <= 0) { toast("数量必须大于 0", "err"); return; }
      setPending(true);
      try {
        await StationeryApi.createPurchase({
          productId: product.id,
          date,
          qty,
          price,
        });
        await Promise.all([refreshProducts(), refreshPurchases()]);
        toast(`已入库 ${qty} 件 · ${product.name}`);
        setProductId(""); setQty(1); setPrice(0);
      } catch (err) {
        toast(err?.message || "进货提交失败", "err");
      } finally {
        setPending(false);
      }
    } else {
      if (!newItem.name.trim()) { toast("请输入商品名称", "err"); return; }
      if (qty <= 0) { toast("数量必须大于 0", "err"); return; }
      setPending(true);
      try {
        const created = await StationeryApi.createProduct({ ...newItem, stock: 0 });
        const productId = created?.id;
        if (!productId) throw new Error("后端未返回新品编号");
        await StationeryApi.createPurchase({
          productId,
          date,
          qty,
          price,
        });
        await Promise.all([refreshProducts(), refreshPurchases()]);
        toast(`新品入库 · ${newItem.name} · ${qty} 件`);
        setNewItem({
          id: "",
          name: "", category: categories[0] || "", maker: "", model: "", price: 0,
        });
        setQty(1); setPrice(0);
      } catch (err) {
        toast(err?.message || "新品入库失败", "err");
      } finally {
        setPending(false);
      }
    }
  };

  const subtotal = (qty || 0) * (price || 0);
  const canSubmit = !pending && (mode === "existing"
    ? (product && qty > 0)
    : (newItem.name && newItem.maker && qty > 0));

  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 05 <span className="sep">/</span> NEW PURCHASE</div>
          <h1 className="page-title"><span className="cn-num">伍</span>录入进货</h1>
        </div>
        <div className="page-actions">
          <button className="btn" onClick={() => goTo("purchases")}>查看进货记录 →</button>
        </div>
      </div>

      <div className="toolbar" style={{ marginBottom: 24 }}>
        <button
          className={"btn " + (mode === "existing" ? "btn-primary" : "")}
          disabled={pending}
          onClick={() => setMode("existing")}
        >① 已有商品补货</button>
        <button
          className={"btn " + (mode === "new" ? "btn-primary" : "")}
          disabled={pending}
          onClick={() => setMode("new")}
        >② 新品入库</button>
      </div>

      <div className="action-page">
        <div className="action-form">
          {mode === "existing" ? (
            <div className="form-grid">
              <div className="field full">
                <label>选择商品 · Select Product</label>
                <select value={productId} onChange={e => setProductId(e.target.value)}>
                  <option value="">— 从库存中选择 —</option>
                  {inventory.map(i => (
                    <option key={i.id} value={i.id}>
                      {i.id} · {i.name} · 当前库存 {i.stock}
                    </option>
                  ))}
                </select>
              </div>
              <div className="field">
                <label>进货日期 · Date</label>
                <input type="date" value={date} onChange={e => setDate(e.target.value)} />
              </div>
              <div className="field">
                <label>类别 · Category</label>
                <input value={product?.category || "—"} disabled />
              </div>
              <div className="field">
                <label>进货数量 · Quantity</label>
                <input type="number" min="1" value={qty}
                  onChange={e => setQty(parseInt(e.target.value || "0", 10))} />
              </div>
              <div className="field">
                <label>进货单价 · Cost (¥)</label>
                <input type="number" min="0" step="0.01" value={price}
                  onChange={e => setPrice(parseFloat(e.target.value || "0"))} />
                {product && (
                  <span className="help">
                    零售价 {fmtMoney(product.price)}
                    {price > 0 && (
                      <span className="vermillion"> · 毛利率 {(((product.price - price) / product.price) * 100).toFixed(1)}%</span>
                    )}
                  </span>
                )}
              </div>
            </div>
          ) : (
            <div className="form-grid">
              <div className="field">
                <label>新建编号 · New ID</label>
                <input className="mono" value="后端自动生成" disabled />
              </div>
              <div className="field">
                <label>类别 · Category</label>
                <select value={newItem.category}
                  onChange={e => setNewItem({ ...newItem, category: e.target.value })}>
                  {categories.map(c => <option key={c} value={c}>{c}</option>)}
                </select>
              </div>
              <div className="field full">
                <label>商品名称 · Name</label>
                <input value={newItem.name}
                  onChange={e => setNewItem({ ...newItem, name: e.target.value })} />
              </div>
              <div className="field">
                <label>生产厂家 · Maker</label>
                <input value={newItem.maker}
                  onChange={e => setNewItem({ ...newItem, maker: e.target.value })} />
              </div>
              <div className="field">
                <label>型号 · Model</label>
                <input value={newItem.model}
                  onChange={e => setNewItem({ ...newItem, model: e.target.value })} />
              </div>
              <div className="field">
                <label>零售单价 · Retail Price (¥)</label>
                <input type="number" min="0" step="0.01" value={newItem.price}
                  onChange={e => setNewItem({ ...newItem, price: parseFloat(e.target.value || "0") })} />
              </div>
              <div className="field">
                <label>进货日期 · Date</label>
                <input type="date" value={date} onChange={e => setDate(e.target.value)} />
              </div>
              <div className="field">
                <label>进货数量 · Quantity</label>
                <input type="number" min="1" value={qty}
                  onChange={e => setQty(parseInt(e.target.value || "0", 10))} />
              </div>
              <div className="field">
                <label>进货单价 · Cost (¥)</label>
                <input type="number" min="0" step="0.01" value={price}
                  onChange={e => setPrice(parseFloat(e.target.value || "0"))} />
              </div>
            </div>
          )}
          <div className="action-tip" style={{ marginTop: 24 }}>
            进货操作将自动增加库存 · 生成新进货单号 · 写入账册
          </div>
        </div>

        <div className="action-summary">
          <div>
            <div className="summary-key">进货明细 · PURCHASE</div>
            <div style={{ marginTop: 16, fontFamily: "var(--serif)", fontSize: 22, fontWeight: 600 }}>
              {mode === "existing" ? (product?.name || "尚未选择商品") : (newItem.name || "新品待录入")}
            </div>
            <div style={{ marginTop: 4, fontFamily: "var(--mono)", fontSize: 11, color: "var(--muted-2)" }}>
              {mode === "existing"
                ? (product ? `${product.id} · ${product.maker} · ${product.model}` : "—")
                : `后端自动编号 · ${newItem.maker || "—"} · ${newItem.model || "—"}`}
            </div>
          </div>

          <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
            <div className="summary-row">
              <span className="summary-key">日期</span>
              <span className="summary-val">{date}</span>
            </div>
            <div className="summary-row">
              <span className="summary-key">进货量</span>
              <span className="summary-val">{qty} 件</span>
            </div>
            <div className="summary-row">
              <span className="summary-key">单价</span>
              <span className="summary-val">{fmtMoney(price)}</span>
            </div>
            <div className="summary-row">
              <span className="summary-key">入库后库存</span>
              <span className="summary-val">
                {mode === "existing"
                  ? (product ? (product.stock + qty) + " 件" : "—")
                  : qty + " 件 (新品)"}
              </span>
            </div>
          </div>

          <div>
            <div className="summary-key" style={{ marginBottom: 8 }}>进货成本 · TOTAL</div>
            <div className="summary-total">{fmtMoney(subtotal)}</div>
          </div>

          <button
            className="btn btn-primary"
            style={{ marginTop: "auto", padding: "16px 24px", fontSize: 12 }}
            disabled={!canSubmit}
            onClick={submit}
          >
            {pending ? "处理中…" : "▸ 确认入库"}
          </button>
        </div>
      </div>
    </div>
  );
}

Object.assign(window, { SellView, RestockView });
