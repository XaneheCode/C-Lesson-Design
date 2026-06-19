/* ============================================
   白泽 · Controlled AI Assistant
   ============================================ */

const SALE_DRAFT_PENDING = "pending";
const SALE_DRAFT_CONFIRMING = "confirming";
const SALE_DRAFT_CONFIRMED = "confirmed";

function saleDraftButtonLabel(status) {
  if (status === SALE_DRAFT_CONFIRMED) return "已写入账册";
  if (status === SALE_DRAFT_CONFIRMING) return "写入中…";
  return "确认并写入账册";
}

function BaiZeAssistant({ onDataChanged }) {
  const [open, setOpen] = useState(false);
  const [configured, setConfigured] = useState(false);
  const [checking, setChecking] = useState(false);
  const [busy, setBusy] = useState(false);
  const [apiKey, setApiKey] = useState("");
  const [input, setInput] = useState("");
  const confirmingDraftIdsRef = useRef(new Set());
  const [messages, setMessages] = useState([
    { role: "assistant", text: "我是白泽。可替你查库存、分析销售、生成补货建议，也可以先拟一份销售单等你确认。" },
  ]);

  const addMessage = (role, text, extra = {}) => {
    setMessages(items => [...items, { role, text, ...extra }]);
  };

  const updateDraftStatus = (draftId, status) => {
    setMessages(items => items.map(message => (
      message.draft?.draft_id === draftId
        ? { ...message, draft: { ...message.draft, status } }
        : message
    )));
  };

  const checkStatus = useCallback(async () => {
    setChecking(true);
    try {
      const status = await StationeryApi.getAssistantStatus();
      setConfigured(Boolean(status?.configured));
    } catch {
      setConfigured(false);
    } finally {
      setChecking(false);
    }
  }, []);

  useEffect(() => {
    if (open) checkStatus();
  }, [open, checkStatus]);

  const saveKey = async (event) => {
    event.preventDefault();
    if (!apiKey.trim()) return;
    setBusy(true);
    try {
      await StationeryApi.setAssistantKey(apiKey.trim());
      setApiKey("");
      setConfigured(true);
      addMessage("assistant", "密钥已放入当前 C 服务进程内存。关闭系统后会自动清除。现在可以开始问我。");
    } catch (err) {
      addMessage("assistant", err?.message || "密钥配置失败。", { error: true });
    } finally {
      setBusy(false);
    }
  };

  const ask = async (text = input) => {
    const question = text.trim();
    if (!question || busy) return;
    setInput("");
    addMessage("user", question);
    setBusy(true);
    try {
      const result = await StationeryApi.askAssistant(question);
      addMessage("assistant", result?.message || "白泽没有返回内容。", {
        draft: result?.type === "sale_draft" ? { ...result, status: SALE_DRAFT_PENDING } : null,
      });
    } catch (err) {
      addMessage("assistant", err?.message || "白泽暂时无法响应。", { error: true });
    } finally {
      setBusy(false);
    }
  };

  const confirmSale = async (draftId) => {
    if (busy || confirmingDraftIdsRef.current.has(draftId)) return;
    confirmingDraftIdsRef.current.add(draftId);
    updateDraftStatus(draftId, SALE_DRAFT_CONFIRMING);
    setBusy(true);
    try {
      const result = await StationeryApi.confirmAssistantSale(draftId);
      updateDraftStatus(draftId, SALE_DRAFT_CONFIRMED);
      addMessage("assistant", result?.message || "销售已确认写入。");
      try {
        await onDataChanged?.();
      } catch {}
    } catch (err) {
      updateDraftStatus(draftId, SALE_DRAFT_PENDING);
      addMessage("assistant", err?.message || "销售确认失败。", { error: true });
    } finally {
      confirmingDraftIdsRef.current.delete(draftId);
      setBusy(false);
    }
  };

  const shortcuts = [
    "查一下库存低于20的商品。",
    "根据库存和销量推荐补货。",
    "统计近一周橡皮销售额和当前库存。",
    "生成一段经营周报。",
  ];

  return (
    <div className={"assistant-dock" + (open ? " open" : "")}>
      {open && (
        <section className="assistant-panel" aria-label="白泽智能助手">
          <header className="assistant-head">
            <div className="assistant-head-mark">
              <img src="assets/baize-assistant.png" alt="" />
            </div>
            <div>
              <div className="assistant-kicker">BAI ZE · 店务智能体</div>
              <h2>白泽问策</h2>
            </div>
            <button className="assistant-close" onClick={() => setOpen(false)} aria-label="关闭白泽助手">×</button>
          </header>

          {!configured ? (
            <form className="assistant-key-form" onSubmit={saveKey}>
              <div className="assistant-key-title">启用千问 3.6 Flash</div>
              <p>每次启动后输入一次新密钥。密钥仅保存在 C 服务内存中，关闭系统即清除，不写入文件。</p>
              <input
                type="password"
                value={apiKey}
                onChange={e => setApiKey(e.target.value)}
                placeholder="输入新的阿里云百炼 API Key"
                autoComplete="off"
              />
              <button className="assistant-primary" disabled={busy || checking}>
                {busy ? "配置中…" : "仅在本次启动中使用"}
              </button>
            </form>
          ) : (
            <>
              <div className="assistant-messages">
                {messages.map((message, index) => (
                  <div className={"assistant-message " + message.role + (message.error ? " error" : "")} key={index}>
                    <div>{message.text}</div>
                    {message.draft && (
                      <div className="assistant-sale-card">
                        <div className="assistant-sale-title">待确认销售单 · {message.draft.draft_id}</div>
                        {message.draft.items.map(item => (
                          <div className="assistant-sale-row" key={item.product_id}>
                            <span>{item.name} × {item.quantity}</span>
                            <strong>{fmtMoney(item.total)}</strong>
                            <small>库存 {item.stock_before} → {item.stock_after}</small>
                          </div>
                        ))}
                        <div className="assistant-sale-total">合计 <strong>{fmtMoney(message.draft.total)}</strong></div>
                        <button
                          className="assistant-primary"
                          onClick={() => confirmSale(message.draft.draft_id)}
                          disabled={busy || message.draft.status !== SALE_DRAFT_PENDING}
                        >
                          {saleDraftButtonLabel(message.draft.status)}
                        </button>
                      </div>
                    )}
                  </div>
                ))}
                {busy && <div className="assistant-thinking">白泽正在推演账册…</div>}
              </div>

              <div className="assistant-shortcuts">
                {shortcuts.map(text => <button key={text} onClick={() => ask(text)}>{text}</button>)}
              </div>

              <form className="assistant-compose" onSubmit={event => { event.preventDefault(); ask(); }}>
                <textarea
                  value={input}
                  onChange={e => setInput(e.target.value)}
                  placeholder="问库存、销售、补货或经营情况…"
                  rows="2"
                />
                <button className="assistant-primary" disabled={busy || !input.trim()}>询问白泽</button>
              </form>
            </>
          )}
        </section>
      )}

      <button
        className="assistant-fab"
        onClick={() => setOpen(value => !value)}
        aria-label={open ? "收起白泽助手" : "打开白泽助手"}
      >
        <img src="assets/baize-assistant.png" alt="" />
      </button>
    </div>
  );
}

window.BaiZeAssistant = BaiZeAssistant;
