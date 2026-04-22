# NW_STA_II_HRF-LC SDK 迁移说明（方案 B：zb205 → zb206/ZB204）

本次自动化迁移把 `NW_STA_II_HRF-LC` 的 SDK 底座从旧版（zb205 + IAR 工程 + 外置 HAL 源码）替换为
参考仓 `new/` 中的新 SDK（zb206/ZB204 + GCC `Makefile`/`build.bat` + `iar_hrf_lib/` 预编译 HAL 库）。

> ⚠️ **重要说明**：本次仅完成机械层面的文件级迁移（add/replace/remove/move），
> 源文件内部的 API 兼容性未做修改，**代码当前不可直接编译通过**，
> 需在本地工具链上按下方"人工收尾事项"逐项处理后，方可出正式版本。

---

## 1. 已经机械完成的变更

### 1.1 SDK 底座替换（new/ → NW_STA_II_HRF-LC/）
将 `new/` 整个目录树以 rsync 方式叠加到 `NW_STA_II_HRF-LC/`，同名文件一律以 `new/` 版本为准。
- 新增构建入口：`Makefile`、`build.bat`、`ZB204.svd`、`iar_hrf_lib/`（HAL 预编译库）、`maf/`、`CCO版本发布.md`
- 新增源码目录：`src/cco/`、`src/lib/`、`src/pack2lib/`、`src/test/`、`src/Protocol_includes.h`
- 新增 ICF/LD：`icf/app.icf`、`icf/debug.icf`、`icf/flash.icf`、`icf/ram.icf`、`icf/sram.ld`、`icf/sram_size.icf`
- 覆盖 PHY/驱动：`src/phy/*`、`src/encry/aes.h`、`ucosIII/uC-CPU/ARM-Cortex-M4/IAR/cpu.h`、`app/app.c`、`app/os_app_hooks.c`、`app/os_cfg_app.h`
- 覆盖公共库扁平化：`src/common/` 新增 `algorithm.{c,h}`、`common_includes.{c,h}`、`fsm.{c,h}`、`list.{c,h}`、`map.{c,h}`、`rbtree.{c,h}`、`timer.{c,h}`、`tmr_call.{c,h}`

### 1.2 芯片库替换
- 删除：`mac_zb205.a`、`phy_zb205.a`
- 新增：`MAC_zb206.a`、`phy_zb206.a`

### 1.3 删除已被新 SDK 取代的旧目录
| 删除路径 | 说明 |
|---|---|
| `lzma2106/` | 新 SDK 不再使用该压缩库 |
| `iar_pack/` | 新 SDK 改用 `Makefile` + `build.bat` 构建，IAR 工程文件不再维护 |
| `ucosIII/uC-Serial/` | 新 SDK 的 uC/OS-III 发行版已剔除 uC-Serial |
| `config/` | 原 `board_sta_def.h` 等已整合至新 SDK 的 `src/common/common_includes.h` 或 `iar_hrf_lib/` 内部 |
| `device/` | 原外置 HAL 源码（GSMCU、HRF、BSP 等）已改为 `iar_hrf_lib/` 提供的预编译库 |

### 1.4 保留并搬迁的 NW 特有业务文件
以下文件在 `new/` 中不存在，属 NW_STA_II_HRF-LC 业务代码，已**原样保留**：
- `src/aps/src/` 下：`lc_app.c`、`lc_obj_model.c`、`lc_whi_adapt.c`、`lc_whi_group.c`、
  `lc_whi_pro.c`、`lc_whi_sysctrl.c`、`lcip_applocal.c`、`crclib.c`、`stream_buffer.c`
- `src/aps/inc/` 下：同名头文件、`lc_all.h`
- `src/lc_iot/`（含 `sta/ble_net/` 等子目录）
- `src/lcdiff/`
- `src/app/src/debug_cmd.c`、`src/app/src/init.c`（含 NW 业务）与 `src/app/inc/init.h`
- `src/app/src/app_*.c`、`src/aps/src/app_*.c`（与 new 同名但含 NW 改动的文件）

搬迁：
- `device/hrf/board/BSP/bps_uart_lcip.{c,h}` → `src/bsp/bps_uart_lcip.{c,h}`

---

## 2. ⚠️ 人工收尾事项（必须完成后才能编译过）

### 2.1 解决"新旧两套同名文件共存"冲突
overlay 产生了 7 组文件冲突，需手工合并（把 NW 原 subdir 版本中的业务逻辑并入 new 的扁平版本，然后删除旧路径）：

| 新 SDK（扁平，保留） | NW 旧（subdir，含业务改动，需合入后删除） |
|---|---|
| `src/aps/applocal.c` | `src/aps/src/applocal.c` |
| `src/aps/applocal.h` | `src/aps/inc/applocal.h` |
| `src/aps/local_protocol.c` | `src/aps/src/local_protocol.c` |
| `src/aps/local_protocol.h` | `src/aps/inc/local_protocol.h` |
| `src/app/init.c` | `src/app/src/init.c` |
| `src/app/init.h` | `src/app/inc/init.h` |
| `src/Protocol_includes.h` | `src/protocol_includes.h` |

建议流程：
1. `diff -u <新扁平> <NW旧subdir>` 查看差异
2. 以扁平版本为基，择优把 NW 业务增量合入
3. 删除旧 subdir 路径下的重复文件
4. 更新所有 `#include` 指向扁平路径

`src/common/` 同理：新增扁平文件 `algorithm.*`、`common_includes.*` 等与旧 `src/common/inc/`、`src/common/src/`、`src/common/finsh/` 共存，需确认 finsh 是否继续使用（new 的 `common_includes.h` 是否包含同类替代）。

### 2.2 修复保留业务代码里失效的 include
删除 `device/`、`config/` 后，以下 include 会 "file not found"，需改用新 SDK 提供的对应头文件（多半在 `iar_hrf_lib/` 或 `src/common/common_includes.h` 中）：

| 失效 include | 出现位置 |
|---|---|
| `#include "bps_flash.h"` | `src/aps/src/lc_obj_model.c`、`src/lc_iot/sta/ble_net/ble_data.c`、`src/lc_iot/sta/ble_net/ble_api.h`、`src/lcdiff/lcdiff_update.h` |
| `#include <bsp.h>` | `src/aps/src/lcip_applocal.c` |
| `#include <gsmcu_hal.h>` | `src/bsp/bps_uart_lcip.c` |
| `#include "bps_timer.h"` | `src/bsp/bps_uart_lcip.c` |

建议：在 `src/common/common_includes.h` 基础上追加一个 NW 专用的桥接头，把旧 API 映射到新 HAL 的 API；或直接把调用改为新 SDK 等价函数。

### 2.3 构建系统切换
原 IAR `OS3.ewp` 已删除。需在 `Makefile` 中：
- 登记上述保留的 NW 业务源文件（`src/aps/src/lc_*.c`、`lcip_applocal.c`、`crclib.c`、`stream_buffer.c`、`src/lc_iot/**/*.c`、`src/lcdiff/**/*.c`、`src/bsp/bps_uart_lcip.c` 等）
- 把 `src/aps/src/`、`src/aps/inc/`、`src/bsp/`、`src/lc_iot/include`、`src/lcdiff/include` 等加到 `-I` 头文件搜索路径
- 确认 ICF：new 默认使用 `icf/sram.icf`（zb206/ZB204 RAM 布局 0x1000_0000-0x100B_FFFF），旧 `gsmcu_app.icf`/`gsmcu_app_debug.icf`/`gsmcu_flash.icf` 目前仍保留在 `icf/` 下，确认不再需要后删除。

### 2.4 API 迁移人工核对
PHY 层 `hplc_phy.c` 从 155 行精简到 37 行、`phy_task.c` 从 256 行扩到 445 行、`phy_port.h`/`hrf_port.h` 大幅重写——属 zb205→zb206 PHY 驱动重构。NW 业务代码若直接调用过旧 PHY 层符号（如 `hrf_driver.c` 中的导出函数），需按新 `src/phy/` 导出的 API 重新对接。

参考 `new/CCO版本发布.md` 的 API/版本记录，核对调用面。

### 2.5 CCO vs STA 变体确认
`new/` 是 CCO 节点版本（见 `CCO版本发布.md`），当前仓库是 STA（`NW_STA_II_HRF-LC`）。
核对 `app/app.c` 与 `src/cco/` 中是否有 CCO 专属流程需替换为 STA 等价实现；
`src/aps/applocal.c`、`src/aps/local_protocol.c` 很可能需要做 CCO/STA 分支裁剪。

---

## 3. 建议验收流程
1. 打开 `build.bat` / `Makefile` 本地编译，逐条解决编译错误
2. 优先按 §2.1 消除重名文件、按 §2.2 修复失效 include
3. 按 §2.4 对接 PHY API
4. 通过编译后，做板级基本冒烟（上电、UART 日志、PHY 收发），再做 LCIP / lc_iot 业务回归
