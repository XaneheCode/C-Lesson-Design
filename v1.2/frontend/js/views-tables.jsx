/* ============================================
   文房 · CRUD Tables (Inventory / Transactions / Purchases)
   ============================================ */

/* ---------- Shared form fields ---------- */
function InventoryForm({ value, onChange, isEdit, categories = CATEGORIES }) {
  const v = value;
  const set = (k, val) => onChange({ ...v, [k]: val });
  return (
    <div className="form-grid">
      <div className="field">
        <label>商品编号 · ID</label>
        <input className="mono" value={isEdit ? v.id : "后端自动生成"} disabled />
        {!isEdit && <span className="help">保存后由后端生成 P001/P002…</span>}
      </div>
      <div className="field">
        <label>类别 · Category</label>
        <select value={v.category} onChange={e => set("category", e.target.value)}>
          {categories.map(c => <option key={c} value={c}>{c}</option>)}
        </select>
      </div>
      <div className="field full">
        <label>商品名称 · Name</label>
        <input value={v.name} onChange={e => set("name", e.target.value)} placeholder="如：晨光中性笔 K35" />
      </div>
      <div className="field">
        <label>生产厂家 · Maker</label>
        <input value={v.maker} onChange={e => set("maker", e.target.value)} />
      </div>
      <div className="field">
        <label>型号 · Model</label>
        <input value={v.model} onChange={e => set("model", e.target.value)} />
      </div>
      <div className="field">
        <label>库存数量 · Stock</label>
        <input type="number" min="0" value={v.stock}
          onChange={e => set("stock", parseInt(e.target.value || "0", 10))} />
      </div>
      <div className="field">
        <label>单价 · Unit Price (¥)</label>
        <input type="number" min="0" step="0.01" value={v.price}
          onChange={e => set("price", parseFloat(e.target.value || "0"))} />
      </div>
    </div>
  );
}

function TransactionForm({ value, onChange, inventory, isEdit }) {
  const v = value;
  const set = (k, val) => onChange({ ...v, [k]: val });
  return (
    <div className="form-grid">
      <div className="field">
        <label>编号 · Transaction ID</label>
        <input className="mono" value={v.id} disabled={isEdit}
          onChange={e => set("id", e.target.value)} />
      </div>
      <div className="field">
        <label>交易日期 · Date</label>
        <input type="date" value={v.date} onChange={e => set("date", e.target.value)} />
      </div>
      <div className="field full">
        <label>商品 · Product</label>
        <select value={v.productId} disabled={isEdit} onChange={e => {
          const item = inventory.find(i => i.id === e.target.value);
          onChange({ ...v, productId: e.target.value,
            name: item?.name || "", category: item?.category || "",
            price: item?.price || v.price });
        }}>
          <option value="">— 选择商品 —</option>
          {inventory.map(i => <option key={i.id} value={i.id}>{i.id} · {i.name}</option>)}
        </select>
      </div>
      <div className="field">
        <label>交易数量 · Qty</label>
        <input type="number" min="1" value={v.qty}
          onChange={e => set("qty", parseInt(e.target.value || "0", 10))} />
      </div>
      <div className="field">
        <label>售价 · Sale Price (¥)</label>
        <input type="number" min="0" step="0.01" value={v.price}
          onChange={e => set("price", parseFloat(e.target.value || "0"))} />
      </div>
    </div>
  );
}

function PurchaseForm({ value, onChange, inventory, isEdit }) {
  const v = value;
  const set = (k, val) => onChange({ ...v, [k]: val });
  return (
    <div className="form-grid">
      <div className="field">
        <label>编号 · Purchase ID</label>
        <input className="mono" value={v.id} disabled={isEdit}
          onChange={e => set("id", e.target.value)} />
      </div>
      <div className="field">
        <label>进货日期 · Date</label>
        <input type="date" value={v.date} onChange={e => set("date", e.target.value)} />
      </div>
      <div className="field full">
        <label>商品 · Product</label>
        <select value={v.productId} disabled={isEdit} onChange={e => {
          const item = inventory.find(i => i.id === e.target.value);
          onChange({ ...v, productId: e.target.value,
            name: item?.name || "", category: item?.category || "" });
        }}>
          <option value="">— 选择商品 —</option>
          {inventory.map(i => <option key={i.id} value={i.id}>{i.id} · {i.name}</option>)}
        </select>
      </div>
      <div className="field">
        <label>进货数量 · Qty</label>
        <input type="number" min="1" value={v.qty}
          onChange={e => set("qty", parseInt(e.target.value || "0", 10))} />
      </div>
      <div className="field">
        <label>进货单价 · Cost (¥)</label>
        <input type="number" min="0" step="0.01" value={v.price}
          onChange={e => set("price", parseFloat(e.target.value || "0"))} />
      </div>
    </div>
  );
}

/* ---------- Inventory View ---------- */
function InventoryView({ inventory, setInventory, refreshProducts, categories = CATEGORIES, loading = false }) {
  const toast = useToast();
  const [q, setQ] = useState("");
  const [cat, setCat] = useState("");
  const [sort, setSort] = useState({ field: "id", dir: "asc" });
  const [editing, setEditing] = useState(null);
  const [confirm, setConfirm] = useState(null);
  const [pending, setPending] = useState("");
  const isLoading = loading && inventory.length === 0;

  const filtered = useMemo(() => {
    let r = inventory;
    if (q) {
      const Q = q.toLowerCase();
      r = r.filter(i =>
        i.id.toLowerCase().includes(Q) ||
        i.name.toLowerCase().includes(Q) ||
        i.maker.toLowerCase().includes(Q) ||
        (i.model || "").toLowerCase().includes(Q)
      );
    }
    if (cat) r = r.filter(i => i.category === cat);
    return sortBy(r, sort.field, sort.dir);
  }, [inventory, q, cat, sort]);
  const pagination = useTablePagination(filtered, `${q}|${cat}|${sort.field}|${sort.dir}`);

  const totals = useMemo(() => ({
    sku: inventory.length,
    units: inventory.reduce((s, i) => s + i.stock, 0),
    value: inventory.reduce((s, i) => s + i.stock * i.price, 0),
    low: inventory.filter(i => i.stock < 20).length,
  }), [inventory]);

  const startCreate = () => setEditing({
    id: "",
    name: "", category: categories[0] || "", maker: "", model: "", stock: 0, price: 0,
    __mode: "create",
  });
  const startEdit = (row) => setEditing({ ...row, __mode: "edit" });
  const save = async () => {
    if (!editing.name.trim()) { toast("请输入商品名称", "err"); return; }
    setPending("save");
    try {
      if (editing.__mode === "create") {
        await StationeryApi.createProduct(editing);
        toast("已新增 · " + editing.name);
      } else {
        await StationeryApi.updateProduct(editing.id, editing);
        toast("已更新 · " + editing.name);
      }
      await refreshProducts();
      setEditing(null);
    } catch (err) {
      toast(err?.message || "保存商品失败", "err");
    } finally {
      setPending("");
    }
  };
  const remove = (row) => setConfirm(row);
  const doRemove = async () => {
    if (!confirm) return;
    setPending("delete");
    try {
      await StationeryApi.deleteProduct(confirm.id);
      await refreshProducts();
      toast("已删除 · " + confirm.name);
      setConfirm(null);
    } catch (err) {
      toast(err?.message || "删除商品失败", "err");
    } finally {
      setPending("");
    }
  };

  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 01 <span className="sep">/</span> INVENTORY</div>
          <h1 className="page-title"><span className="cn-num">壹</span>库存管理</h1>
        </div>
        <div className="page-actions">
          <button className="btn btn-primary" onClick={startCreate}>+ 新增库存</button>
        </div>
      </div>

      {isLoading ? <StatsSkeleton /> : <div className="stats-strip">
        <div className="stat-cell">
          <div className="stat-label">SKU 种类</div>
          <div className="stat-value">{totals.sku}<span className="unit">种</span></div>
          <div className="stat-foot">across {categories.length} categories</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">总库存量</div>
          <div className="stat-value">{fmtInt(totals.units)}<span className="unit">件</span></div>
          <div className="stat-foot">total units on hand</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">库存总值</div>
          <div className="stat-value">{fmtMoney(totals.value)}</div>
          <div className="stat-foot">at retail price</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">低库存预警</div>
          <div className="stat-value vermillion">{totals.low}<span className="unit">项</span></div>
          <div className="stat-foot down">stock &lt; 20</div>
        </div>
      </div>}

      <div className="toolbar">
        <div className="tb-search">
          <span className="icn">⌕</span>
          <input value={q} onChange={e => setQ(e.target.value)}
            placeholder="按编号 / 名称 / 厂家 / 型号 查询…" />
        </div>
        <select className="tb-select" value={cat} onChange={e => setCat(e.target.value)}>
          <option value="">全部类别</option>
          {categories.map(c => <option key={c} value={c}>{c}</option>)}
        </select>
        <button className="btn btn-sm" onClick={() => { setQ(""); setCat(""); }}>清除筛选</button>
      </div>

      <div className="tbl-wrap">
        <table className="tbl">
          <thead>
            <tr>
              <Th field="id"       sort={sort} setSort={setSort}>编号</Th>
              <Th field="name"     sort={sort} setSort={setSort}>商品名称</Th>
              <Th field="category" sort={sort} setSort={setSort}>类别</Th>
              <Th field="maker"    sort={sort} setSort={setSort}>厂家</Th>
              <Th field="model"    sort={sort} setSort={setSort}>型号</Th>
              <Th field="stock"    sort={sort} setSort={setSort} align="right">库存</Th>
              <Th field="price"    sort={sort} setSort={setSort} align="right">单价</Th>
              <th style={{ textAlign: "right" }}>操作</th>
            </tr>
          </thead>
          <tbody>
            {isLoading ? (
              <TableSkeletonRows rows={6} cols={8} />
            ) : filtered.length === 0 && (
              <tr><td colSpan="8" className="empty">无匹配记录 · NO RESULTS</td></tr>
            )}
            {!isLoading && pagination.pageItems.map(row => (
              <tr key={row.id}>
                <td className="id-cell">{row.id}</td>
                <td className="name-cell">{row.name}</td>
                <td><CategoryTag value={row.category} /></td>
                <td>{row.maker}</td>
                <td className="mono" style={{ fontSize: 12 }}>{row.model}</td>
                <td className="num-cell" style={ row.stock < 20 ? { color: "var(--vermillion)" } : null }>
                  {fmtInt(row.stock)}
                </td>
                <td className="num-cell">{fmtMoney(row.price)}</td>
                <td className="actions-cell">
                  <button className="btn btn-ghost btn-sm" disabled={!!pending} onClick={() => startEdit(row)}>修改</button>
                  <button className="btn btn-ghost btn-sm" disabled={!!pending} onClick={() => remove(row)} style={{ color: "var(--vermillion)" }}>删除</button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      <Pagination
        page={pagination.page}
        pageCount={pagination.pageCount}
        total={pagination.total}
        pageSize={pagination.pageSize}
        onPageChange={pagination.setPage}
      />

      <Modal
        open={!!editing}
        onClose={() => setEditing(null)}
        title={editing?.__mode === "create" ? "新增商品" : "修改商品"}
        sub={editing?.__mode === "create" ? "Add Inventory Item" : "Edit Inventory Item"}
        footer={
          <>
            <button className="btn" onClick={() => setEditing(null)} disabled={pending === "save"}>取消</button>
            <button className="btn btn-primary" onClick={save} disabled={pending === "save"}>
              {pending === "save" ? "处理中…" : "保存"}
            </button>
          </>
        }
      >
        {editing && <InventoryForm value={editing} onChange={setEditing} isEdit={editing.__mode === "edit"} categories={categories} />}
      </Modal>

      <Confirm
        open={!!confirm}
        title="删除商品"
        message={`确认要将该商品从库存中移除？相关交易/进货记录不会被自动删除。`}
        target={confirm && `${confirm.id} · ${confirm.name}`}
        onCancel={() => setConfirm(null)}
        onConfirm={doRemove}
        busy={pending === "delete"}
      />
    </div>
  );
}

/* ---------- Transactions View ---------- */
function TransactionsView({ transactions, setTransactions, inventory, refreshTransactions, refreshProducts, categories = CATEGORIES, loading = false }) {
  const toast = useToast();
  const [q, setQ] = useState("");
  const [cat, setCat] = useState("");
  const [sort, setSort] = useState({ field: "date", dir: "desc" });
  const [editing, setEditing] = useState(null);
  const [confirm, setConfirm] = useState(null);
  const [pending, setPending] = useState("");
  const isLoading = loading && transactions.length === 0;

  const filtered = useMemo(() => {
    let r = transactions;
    if (q) {
      const Q = q.toLowerCase();
      r = r.filter(t =>
        t.id.toLowerCase().includes(Q) ||
        t.productId.toLowerCase().includes(Q) ||
        t.name.toLowerCase().includes(Q)
      );
    }
    if (cat) r = r.filter(t => t.category === cat);
    return sortBy(r, sort.field, sort.dir);
  }, [transactions, q, cat, sort]);
  const pagination = useTablePagination(filtered, `${q}|${cat}|${sort.field}|${sort.dir}`);

  const totals = useMemo(() => ({
    count: transactions.length,
    units: transactions.reduce((s, t) => s + t.qty, 0),
    revenue: transactions.reduce((s, t) => s + t.qty * t.price, 0),
    avg: transactions.length ? transactions.reduce((s, t) => s + t.qty * t.price, 0) / transactions.length : 0,
  }), [transactions]);

  const startEdit = (row) => setEditing({ ...row, __mode: "edit" });
  const save = async () => {
    if (!editing) return;
    if (editing.qty <= 0) { toast("数量必须大于 0", "err"); return; }
    setPending("save");
    try {
      await StationeryApi.updateSale(editing.id, editing);
      await Promise.all([refreshTransactions(), refreshProducts()]);
      toast("已更新交易 · " + editing.id);
      setEditing(null);
    } catch (err) {
      toast(err?.message || "更新交易失败", "err");
    } finally {
      setPending("");
    }
  };
  const remove = (row) => setConfirm(row);
  const doRemove = async () => {
    if (!confirm) return;
    setPending("delete");
    try {
      await StationeryApi.deleteSale(confirm.id);
      await Promise.all([refreshTransactions(), refreshProducts()]);
      toast("已删除交易 · " + confirm.id);
      setConfirm(null);
    } catch (err) {
      toast(err?.message || "删除交易失败", "err");
    } finally {
      setPending("");
    }
  };

  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 02 <span className="sep">/</span> TRANSACTIONS</div>
          <h1 className="page-title"><span className="cn-num">贰</span>交易记录</h1>
        </div>
        <div className="page-actions">
          <button className="btn" onClick={() => setSort({ field: "date", dir: "desc" })}>按日期降序</button>
          <button className="btn" onClick={() => setSort({ field: "date", dir: "asc" })}>按日期升序</button>
        </div>
      </div>

      {isLoading ? <StatsSkeleton /> : <div className="stats-strip">
        <div className="stat-cell">
          <div className="stat-label">交易笔数</div>
          <div className="stat-value">{totals.count}<span className="unit">笔</span></div>
          <div className="stat-foot">in current ledger</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">售出数量</div>
          <div className="stat-value">{fmtInt(totals.units)}<span className="unit">件</span></div>
          <div className="stat-foot">total units sold</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">销售总额</div>
          <div className="stat-value vermillion">{fmtMoney(totals.revenue)}</div>
          <div className="stat-foot up">gross revenue</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">单笔均值</div>
          <div className="stat-value">{fmtMoney(totals.avg)}</div>
          <div className="stat-foot">avg ticket</div>
        </div>
      </div>}

      <div className="toolbar">
        <div className="tb-search">
          <span className="icn">⌕</span>
          <input value={q} onChange={e => setQ(e.target.value)}
            placeholder="按交易号 / 商品编号 / 商品名称 查询…" />
        </div>
        <select className="tb-select" value={cat} onChange={e => setCat(e.target.value)}>
          <option value="">全部类别</option>
          {categories.map(c => <option key={c} value={c}>{c}</option>)}
        </select>
        <button className="btn btn-sm" onClick={() => { setQ(""); setCat(""); }}>清除筛选</button>
      </div>

      <div className="tbl-wrap">
        <table className="tbl">
          <thead>
            <tr>
              <Th field="id"        sort={sort} setSort={setSort}>交易号</Th>
              <Th field="date"      sort={sort} setSort={setSort}>交易日期</Th>
              <Th field="productId" sort={sort} setSort={setSort}>商品编号</Th>
              <Th field="name"      sort={sort} setSort={setSort}>商品名称</Th>
              <Th field="category"  sort={sort} setSort={setSort}>类别</Th>
              <Th field="qty"       sort={sort} setSort={setSort} align="right">数量</Th>
              <Th field="price"     sort={sort} setSort={setSort} align="right">售价</Th>
              <th style={{ textAlign: "right" }}>小计</th>
              <th style={{ textAlign: "right" }}>操作</th>
            </tr>
          </thead>
          <tbody>
            {isLoading ? (
              <TableSkeletonRows rows={6} cols={9} />
            ) : filtered.length === 0 && (
              <tr><td colSpan="9" className="empty">无匹配记录</td></tr>
            )}
            {!isLoading && pagination.pageItems.map(row => (
              <tr key={row.id}>
                <td className="id-cell">{row.id}</td>
                <td className="mono" style={{ fontSize: 12 }}>{row.date}</td>
                <td className="id-cell">{row.productId}</td>
                <td className="name-cell">{row.name}</td>
                <td><CategoryTag value={row.category} /></td>
                <td className="num-cell">{fmtInt(row.qty)}</td>
                <td className="num-cell">{fmtMoney(row.price)}</td>
                <td className="num-cell" style={{ fontWeight: 600 }}>{fmtMoney(row.qty * row.price)}</td>
                <td className="actions-cell">
                  <button className="btn btn-ghost btn-sm" disabled={!!pending} onClick={() => startEdit(row)}>修改</button>
                  <button className="btn btn-ghost btn-sm" disabled={!!pending} onClick={() => remove(row)} style={{ color: "var(--vermillion)" }}>删除</button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      <Pagination
        page={pagination.page}
        pageCount={pagination.pageCount}
        total={pagination.total}
        pageSize={pagination.pageSize}
        onPageChange={pagination.setPage}
      />

      <Modal
        open={!!editing}
        onClose={() => setEditing(null)}
        title="修改交易"
        sub="Edit Transaction"
        footer={
          <>
            <button className="btn" onClick={() => setEditing(null)} disabled={pending === "save"}>取消</button>
            <button className="btn btn-primary" onClick={save} disabled={pending === "save"}>
              {pending === "save" ? "处理中…" : "保存"}
            </button>
          </>
        }
      >
        {editing && <TransactionForm value={editing} onChange={setEditing} inventory={inventory} isEdit />}
      </Modal>

      <Confirm
        open={!!confirm}
        title="删除交易"
        message="该笔交易将从账册中移除，后端会自动恢复对应库存。"
        target={confirm && `${confirm.id} · ${confirm.name} · ${fmtMoney((confirm.qty || 0) * (confirm.price || 0))}`}
        onCancel={() => setConfirm(null)}
        onConfirm={doRemove}
        busy={pending === "delete"}
      />
    </div>
  );
}

/* ---------- Purchases View ---------- */
function PurchasesView({ purchases, setPurchases, inventory, refreshPurchases, refreshProducts, categories = CATEGORIES, loading = false }) {
  const toast = useToast();
  const [q, setQ] = useState("");
  const [cat, setCat] = useState("");
  const [sort, setSort] = useState({ field: "date", dir: "desc" });
  const [editing, setEditing] = useState(null);
  const [confirm, setConfirm] = useState(null);
  const [pending, setPending] = useState("");
  const isLoading = loading && purchases.length === 0;

  const filtered = useMemo(() => {
    let r = purchases;
    if (q) {
      const Q = q.toLowerCase();
      r = r.filter(t =>
        t.id.toLowerCase().includes(Q) ||
        t.productId.toLowerCase().includes(Q) ||
        t.name.toLowerCase().includes(Q)
      );
    }
    if (cat) r = r.filter(t => t.category === cat);
    return sortBy(r, sort.field, sort.dir);
  }, [purchases, q, cat, sort]);
  const pagination = useTablePagination(filtered, `${q}|${cat}|${sort.field}|${sort.dir}`);

  const totals = useMemo(() => ({
    count: purchases.length,
    units: purchases.reduce((s, t) => s + t.qty, 0),
    cost: purchases.reduce((s, t) => s + t.qty * t.price, 0),
  }), [purchases]);

  const startEdit = (row) => setEditing({ ...row, __mode: "edit" });
  const save = async () => {
    if (!editing) return;
    if (editing.qty <= 0) { toast("数量必须大于 0", "err"); return; }
    setPending("save");
    try {
      await StationeryApi.updatePurchase(editing.id, editing);
      await Promise.all([refreshPurchases(), refreshProducts()]);
      toast("已更新进货 · " + editing.id);
      setEditing(null);
    } catch (err) {
      toast(err?.message || "更新进货失败", "err");
    } finally {
      setPending("");
    }
  };
  const remove = (row) => setConfirm(row);
  const doRemove = async () => {
    if (!confirm) return;
    setPending("delete");
    try {
      await StationeryApi.deletePurchase(confirm.id);
      await Promise.all([refreshPurchases(), refreshProducts()]);
      toast("已删除进货 · " + confirm.id);
      setConfirm(null);
    } catch (err) {
      toast(err?.message || "删除进货失败", "err");
    } finally {
      setPending("");
    }
  };

  return (
    <div>
      <div className="page-head">
        <div className="page-head-left">
          <div className="crumb">FUNCTION 03 <span className="sep">/</span> PURCHASES</div>
          <h1 className="page-title"><span className="cn-num">叁</span>进货记录</h1>
        </div>
      </div>

      {isLoading ? <StatsSkeleton /> : <div className="stats-strip">
        <div className="stat-cell">
          <div className="stat-label">进货单数</div>
          <div className="stat-value">{totals.count}<span className="unit">单</span></div>
          <div className="stat-foot">in current ledger</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">入库数量</div>
          <div className="stat-value">{fmtInt(totals.units)}<span className="unit">件</span></div>
          <div className="stat-foot">total units received</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">进货成本</div>
          <div className="stat-value">{fmtMoney(totals.cost)}</div>
          <div className="stat-foot">total spend</div>
        </div>
        <div className="stat-cell">
          <div className="stat-label">平均单价</div>
          <div className="stat-value">{fmtMoney(totals.units ? totals.cost / totals.units : 0)}</div>
          <div className="stat-foot">per unit</div>
        </div>
      </div>}

      <div className="toolbar">
        <div className="tb-search">
          <span className="icn">⌕</span>
          <input value={q} onChange={e => setQ(e.target.value)}
            placeholder="按单号 / 商品编号 / 商品名称 查询…" />
        </div>
        <select className="tb-select" value={cat} onChange={e => setCat(e.target.value)}>
          <option value="">全部类别</option>
          {categories.map(c => <option key={c} value={c}>{c}</option>)}
        </select>
        <button className="btn btn-sm" onClick={() => { setQ(""); setCat(""); }}>清除筛选</button>
      </div>

      <div className="tbl-wrap">
        <table className="tbl">
          <thead>
            <tr>
              <Th field="id"        sort={sort} setSort={setSort}>单号</Th>
              <Th field="date"      sort={sort} setSort={setSort}>进货日期</Th>
              <Th field="productId" sort={sort} setSort={setSort}>商品编号</Th>
              <Th field="name"      sort={sort} setSort={setSort}>商品名称</Th>
              <Th field="category"  sort={sort} setSort={setSort}>类别</Th>
              <Th field="qty"       sort={sort} setSort={setSort} align="right">数量</Th>
              <Th field="price"     sort={sort} setSort={setSort} align="right">单价</Th>
              <th style={{ textAlign: "right" }}>合计</th>
              <th style={{ textAlign: "right" }}>操作</th>
            </tr>
          </thead>
          <tbody>
            {isLoading ? (
              <TableSkeletonRows rows={6} cols={9} />
            ) : filtered.length === 0 && (
              <tr><td colSpan="9" className="empty">无匹配记录</td></tr>
            )}
            {!isLoading && pagination.pageItems.map(row => (
              <tr key={row.id}>
                <td className="id-cell">{row.id}</td>
                <td className="mono" style={{ fontSize: 12 }}>{row.date}</td>
                <td className="id-cell">{row.productId}</td>
                <td className="name-cell">{row.name}</td>
                <td><CategoryTag value={row.category} /></td>
                <td className="num-cell">{fmtInt(row.qty)}</td>
                <td className="num-cell">{fmtMoney(row.price)}</td>
                <td className="num-cell" style={{ fontWeight: 600 }}>{fmtMoney(row.qty * row.price)}</td>
                <td className="actions-cell">
                  <button className="btn btn-ghost btn-sm" disabled={!!pending} onClick={() => startEdit(row)}>修改</button>
                  <button className="btn btn-ghost btn-sm" disabled={!!pending} onClick={() => remove(row)} style={{ color: "var(--vermillion)" }}>删除</button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
      <Pagination
        page={pagination.page}
        pageCount={pagination.pageCount}
        total={pagination.total}
        pageSize={pagination.pageSize}
        onPageChange={pagination.setPage}
      />

      <Modal
        open={!!editing}
        onClose={() => setEditing(null)}
        title="修改进货"
        sub="Edit Purchase"
        footer={
          <>
            <button className="btn" onClick={() => setEditing(null)} disabled={pending === "save"}>取消</button>
            <button className="btn btn-primary" onClick={save} disabled={pending === "save"}>
              {pending === "save" ? "处理中…" : "保存"}
            </button>
          </>
        }
      >
        {editing && <PurchaseForm value={editing} onChange={setEditing} inventory={inventory} isEdit />}
      </Modal>

      <Confirm
        open={!!confirm}
        title="删除进货"
        message="该进货记录将从账册中移除，后端会自动扣回对应库存。"
        target={confirm && `${confirm.id} · ${confirm.name}`}
        onCancel={() => setConfirm(null)}
        onConfirm={doRemove}
        busy={pending === "delete"}
      />
    </div>
  );
}

Object.assign(window, {
  InventoryView, TransactionsView, PurchasesView,
  InventoryForm, TransactionForm, PurchaseForm,
});
