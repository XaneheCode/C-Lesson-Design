/* ============================================
   文房 · Welcome Screen
   ============================================ */

function WelcomeView({ onEnter, inventory, transactions, purchases = [] }) {
  // Compute live stats for the welcome
  const totalItems = inventory.length;
  const totalStock = inventory.reduce((s, i) => s + i.stock, 0);
  const todaySales = transactions
    .filter(t => t.date === today(0))
    .reduce((s, t) => s + t.qty * t.price, 0);

  const tiles = [
    { id: "inventory",    num: "01", label: "库存管理", sub: `Inventory · ${inventory.length} SKU` , feature: false },
    { id: "transactions", num: "02", label: "交易记录", sub: `Transactions · ${transactions.length}`, feature: false },
    { id: "purchases",    num: "03", label: "进货管理", sub: `Purchases · ${purchases.length}`,      feature: false },
    { id: "sell",         num: "04", label: "新建销售", sub: "New Sale",            feature: true  },
    { id: "restock",      num: "05", label: "录入进货", sub: "New Purchase",        feature: false },
    { id: "query",        num: "06", label: "多条件查询", sub: "Advanced Query",    feature: false },
    { id: "stats",        num: "07", label: "经营统计", sub: "Analytics",           feature: true  },
    { id: "help",         num: "08", label: "使用说明", sub: "Documentation",       feature: false },
  ];

  return (
    <div className="welcome">
      <div className="welcome-top">
        <span className="stamp">EST · MMXXVI · 文房</span>
        <span>VOL. 01 / NO. 05 / 2026.05.25</span>
        <span>STATIONERY ADMIN SUITE</span>
      </div>

      <div className="welcome-hero">
        <div className="hero-left">
          <h1 className="hero-title">
            <span className="ch">文<span className="seal">藏</span></span>
            <span className="ch">房</span>
          </h1>
          <dl className="hero-meta">
            <dt>System</dt><dd>Stationery Sales & Inventory · v1.0</dd>
            <dt>Today</dt><dd>{today(0).replaceAll("-", ".")} · LIVE</dd>
            <dt>Stock</dt><dd>{fmtInt(totalStock)} units across {totalItems} SKU</dd>
            <dt>Status</dt><dd className="vermillion">● Live · synced</dd>
          </dl>
        </div>
        <div className="hero-right">
          <span className="quote-mark">"</span>
          于一笔一本之间，照见百业经营之道。库存有数，进销有节，所行所记，皆成章法。
          <em>— 卷首 · 文房记</em>
        </div>
      </div>

      <div className="func-grid">
        {tiles.map(t => (
          <button
            key={t.id}
            className={"func-tile" + (t.feature ? " feature" : "")}
            onClick={() => onEnter(t.id)}
          >
            <div className="tile-top">
              <span className="tile-num">{t.num}</span>
              <span className="tile-arrow">→</span>
            </div>
            <div className="tile-bottom">
              <div className="tile-label">{t.label}</div>
              <div className="tile-sub">{t.sub}</div>
            </div>
          </button>
        ))}
      </div>

      <div className="welcome-bottom">
        <span>← 点击任一模块进入 · CLICK ANY TILE TO ENTER</span>
        <span className="vol">今日入账 · {fmtMoney(todaySales)}</span>
        <span>键盘可用 · KEYBOARD READY</span>
      </div>
    </div>
  );
}

window.WelcomeView = WelcomeView;
