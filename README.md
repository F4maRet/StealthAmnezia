# StealthAmnezia

**Самый необнаруживаемый self-hosted VPN 2026 года**

Только два протокола:
- **AmneziaWG 2.0** (WireGuard + автоматизированная CPS-обфускация, junk-train, динамические заголовки H1–H4, S1–S4)
- **XRay VLESS + Reality** (uTLS-фингерпринты + fallback на реальные сайты)

**Новые фичи (по сравнению с оригиналом Amnezia):**
- Полностью автоматический CPS-генератор + пресеты по странам (РФ, Иран, Китай)
- Авто-обновление Docker-контейнеров (Watchtower + graceful restart)
- Hybrid fallback между AmneziaWG и XRay
- Улучшенный security (sandbox IPC, TPM/Keychain)
- Продвинутый split-tunneling по доменам

**Как установить сервер:**
1. Запусти клиент → «Добавить сервер» → SSH (IP + логин/пароль)
2. Всё остальное — автоматически

Поддержка: Windows / macOS / Linux / Android / iOS

**Статус:** в разработке (v1.0 — апрель 2026)