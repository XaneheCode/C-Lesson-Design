/* ============================================
   文房 · App Shell
   ============================================ */

const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "accent": "#C8341D",
  "showSealMark": true
}/*EDITMODE-END*/;

const NAV = [
  { group: "账册管理", items: [
    { id: "inventory",    num: "01", label: "库存管理" },
    { id: "transactions", num: "02", label: "交易记录" },
    { id: "purchases",    num: "03", label: "进货记录" },
  ]},
  { group: "业务操作", items: [
    { id: "sell",    num: "04", label: "新建销售" },
    { id: "restock", num: "05", label: "录入进货" },
  ]},
  { group: "分析与查询", items: [
    { id: "query", num: "06", label: "多条件查询" },
    { id: "stats", num: "07", label: "经营统计" },
    { id: "help",  num: "08", label: "使用说明" },
  ]},
];

function ApiErrorNotice({ message, onRetry }) {
  if (!message) return null;
  return (
    <div className="api-error">
      <div>
        <div className="api-error-title">后端连接异常</div>
        <div className="api-error-msg">{message}</div>
      </div>
      <button className="btn btn-sm" onClick={onRetry}>重新加载</button>
    </div>
  );
}

function AppContent() {
  const toast = useToast();
  const [inventory, setInventory]       = useState([]);
  const [transactions, setTransactions] = useState([]);
  const [purchases, setPurchases]       = useState([]);
  const [route, setRoute]               = usePersistedState("wf:route","welcome");
  const [t, setTweak] = useTweaks(TWEAK_DEFAULTS);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");

  // Apply tweaks
  useEffect(() => {
    if (t.accent) document.documentElement.style.setProperty("--vermillion", t.accent);
  }, [t.accent]);

  const refreshProducts = useCallback(async () => {
    const data = await StationeryApi.listProducts();
    setInventory(data);
    return data;
  }, []);

  const refreshSales = useCallback(async () => {
    const data = await StationeryApi.listSales();
    setTransactions(data);
    return data;
  }, []);

  const refreshPurchases = useCallback(async () => {
    const data = await StationeryApi.listPurchases();
    setPurchases(data);
    return data;
  }, []);

  const loadAll = useCallback(async ({ silent = false } = {}) => {
    if (!silent) setLoading(true);
    setError("");
    try {
      const [nextInventory, nextTransactions, nextPurchases] = await Promise.all([
        StationeryApi.listProducts(),
        StationeryApi.listSales(),
        StationeryApi.listPurchases(),
      ]);
      setInventory(nextInventory);
      setTransactions(nextTransactions);
      setPurchases(nextPurchases);
      return { inventory: nextInventory, transactions: nextTransactions, purchases: nextPurchases };
    } catch (err) {
      const msg = err?.message || "数据加载失败";
      setError(msg);
      toast(msg, "err");
      throw err;
    } finally {
      if (!silent) setLoading(false);
    }
  }, [toast]);

  useEffect(() => {
    loadAll().catch(() => {});
  }, [loadAll]);

  const categories = useMemo(() => {
    const dynamic = [...new Set(inventory.map(i => i.category).filter(Boolean))];
    return dynamic.length ? dynamic : CATEGORIES;
  }, [inventory]);

  const resetAll = () => {
    if (confirm("是否重新从后端加载最新数据？")) {
      loadAll()
        .then(() => toast("已重新加载后端数据"))
        .catch(() => {});
    }
  };

  if (route === "welcome") {
    return (
      <>
        <div className="paper-bg" />
        <ApiErrorNotice message={error} onRetry={() => loadAll().catch(() => {})} />
        <WelcomeView
          onEnter={setRoute}
          inventory={inventory}
          transactions={transactions}
          purchases={purchases}
        />
        <TweaksPanel title="Tweaks">
          <TweakSection title="Theme">
            <TweakColor
              label="Accent · 强调色"
              value={t.accent}
              onChange={v => setTweak("accent", v)}
              options={["#C8341D", "#2C5530", "#3B5BA0", "#B08545", "#1F1A14"]}
            />
          </TweakSection>
        </TweaksPanel>
      </>
    );
  }

  return (
    <>
      <div className="paper-bg" />
      <div className="app-shell">
        <aside className="sidebar">
          <div className="sb-brand" onClick={() => setRoute("welcome")}>
            <div className="ch"><span>文</span><span>房</span></div>
            <div className="sb-sub">Stationery · v1.0</div>
          </div>

          {NAV.map(group => (
            <div className="sb-section" key={group.group}>
              <div className="sb-section-title">{group.group}</div>
              {group.items.map(item => (
                <div
                  key={item.id}
                  className={"sb-item" + (route === item.id ? " active" : "")}
                  onClick={() => setRoute(item.id)}
                >
                  <span>{item.label}</span>
                  <span className="num">{item.num}</span>
                </div>
              ))}
            </div>
          ))}

          <div className="sb-foot">
            <div className="live">系统在线 · LIVE</div>
            <div>当日 · {today(0)}</div>
            <div
              style={{ marginTop: 8, cursor: "pointer", color: "var(--paper)" }}
              onClick={resetAll}
            >⟲ 重新加载数据</div>
          </div>
        </aside>

        <main className="main">
          <ApiErrorNotice message={error} onRetry={() => loadAll().catch(() => {})} />
          {route === "inventory" && (
            <InventoryView
              inventory={inventory}
              setInventory={setInventory}
              refreshProducts={refreshProducts}
              categories={categories}
              loading={loading}
            />
          )}
          {route === "transactions" && (
            <TransactionsView
              transactions={transactions}
              setTransactions={setTransactions}
              inventory={inventory}
              refreshTransactions={refreshSales}
              refreshProducts={refreshProducts}
              categories={categories}
              loading={loading}
            />
          )}
          {route === "purchases" && (
            <PurchasesView
              purchases={purchases}
              setPurchases={setPurchases}
              inventory={inventory}
              refreshPurchases={refreshPurchases}
              refreshProducts={refreshProducts}
              categories={categories}
              loading={loading}
            />
          )}
          {route === "sell" && (
            <SellView
              inventory={inventory}
              setInventory={setInventory}
              transactions={transactions}
              setTransactions={setTransactions}
              refreshProducts={refreshProducts}
              refreshTransactions={refreshSales}
              goTo={setRoute}
            />
          )}
          {route === "restock" && (
            <RestockView
              inventory={inventory}
              setInventory={setInventory}
              purchases={purchases}
              setPurchases={setPurchases}
              refreshProducts={refreshProducts}
              refreshPurchases={refreshPurchases}
              categories={categories}
              goTo={setRoute}
            />
          )}
          {route === "query" && (
            <QueryView
              inventory={inventory}
              transactions={transactions}
              purchases={purchases}
              categories={categories}
              loading={loading}
            />
          )}
          {route === "stats" && (
            <StatsView
              inventory={inventory}
              transactions={transactions}
              purchases={purchases}
              categories={categories}
              loading={loading}
            />
          )}
          {route === "help" && <HelpView />}
        </main>
      </div>

      <TweaksPanel title="Tweaks">
        <TweakSection title="Theme">
          <TweakColor
            label="Accent · 强调色"
            value={t.accent}
            onChange={v => setTweak("accent", v)}
            options={["#C8341D", "#2C5530", "#3B5BA0", "#B08545", "#1F1A14"]}
          />
        </TweakSection>
        <TweakSection title="Quick Actions">
          <TweakButton onClick={() => setRoute("welcome")}>返回欢迎页</TweakButton>
          <TweakButton onClick={resetAll}>重新加载后端数据</TweakButton>
        </TweakSection>
      </TweaksPanel>
    </>
  );
}

function App() {
  return (
    <ToastProvider>
      <AppContent />
    </ToastProvider>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);
