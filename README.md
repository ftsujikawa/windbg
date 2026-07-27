# tdb（windbg）

Windows (x64) 上で動作する、小規模なコマンドライン対話型ネイティブデバッガです。
Win32 デバッグ API（`CreateProcess` / `WaitForDebugEvent` / `ContinueDebugEvent` 等）と
DbgHelp ライブラリ（PDB シンボル解析）を使って実装されており、GDB に似たコマンド体系
（`break`, `continue`, `si`, `print` 等）を提供します。

対象プログラムがマルチスレッド・マルチプロセス（対象プログラム自身が生成する子プロセス）の
場合も、生存中の全スレッド・全プロセスを自動的に追跡し、フォーカス（対話コマンドの対象）の
切替・自動固定に対応します。

## ビルド

Visual Studio の Developer Command Prompt（`cl` が使える環境）と、`win_bison`/`win_flex`
（[winflexbison](https://github.com/lexxmark/winflexbison) パッケージ）が `PATH` 上にある
ことが前提です。

```bat
cd windbg
make.bat
```

`make.bat` はコマンド行・式評価器の文法（`cmdline_parser.y`/`cmdline_lexer.l`、
`expr_parser.y`/`expr_lexer.l`）から `win_bison`/`win_flex` でパーサを生成した後、
`tdb.exe` 本体と、動作確認用のサンプル対象プログラム（`testprog.exe` 等）をビルドします。

## 使い方

```bat
tdb.exe <対象プログラム.exe>
```

主なコマンド（`help` でコマンド一覧、`help <command>` で詳細な用法を表示できます）:

| コマンド | 内容 |
|---|---|
| `break` / `b` | ブレークポイント設定 |
| `continue` / `c` | 実行継続 |
| `si` / `step` / `n` / `up` | ステップ実行各種 |
| `regs` / `x` / `dis` / `tb` | レジスタ／メモリ／逆アセンブル／バックトレース表示 |
| `print` / `p` / `set` | 式評価・表示／変数・レジスタ代入 |
| `watch` / `wdel` | ハードウェアウォッチポイント |
| `leak` | malloc/calloc/realloc/free のリーク追跡 |
| `thread` / `show threads` | マルチスレッド時のフォーカス切替・一覧表示 |
| `process` / `show processes` | マルチプロセス時のフォーカス切替・一覧表示 |
| `show` | 変数・ブレークポイント・リーク・ウォッチポイント等の表示 |

対象プログラムが子プロセスを生成した場合、自動的に追従デバッグします。ただしブレークポイント・
ウォッチポイントはプロセスごとに独立しており、親プロセスで設定したものは子プロセスへ自動的には
伝播しません。子プロセス側にも停止したい箇所がある場合は `process <id>` で切り替えたうえで
改めて `break`/`watch` を設定してください。

## テスト

テスト仕様書（[docs/04_テスト仕様書.md](../docs/04_テスト仕様書.md)）のテストケースの多くは
[tests/run_tests.ps1](../tests/run_tests.ps1) で自動実行できます。

```powershell
cd tests
pwsh -File run_tests.ps1
```

PowerShell 7（`pwsh`）で実行してください。UTF-8（BOM なし）で書かれているため、Windows 標準の
`powershell.exe`（5.1 系）では文字コードの解釈違いにより文字化けやパースエラーが発生します。

- `-VerboseOutput`: 各テストの `tdb.exe` 実際の出力全文を表示（失敗時の原因調査用）
- `-Only '<正規表現>'`: ID が一致するテストのみ実行（例: `-Only 'M0[1-9]'`）

コンパイル警告有無の確認（Q01）はフルリビルドを伴うため別スクリプトです（Developer Command
Prompt 環境が必要）:

```powershell
pwsh -File tests\check_build_warnings.ps1
```

## ドキュメント

設計・仕様の詳細は `docs/` 配下を参照してください。

- [01_要件定義書.md](../docs/01_要件定義書.md) — 対象範囲、機能要件、既知の制限
- [02_基本設計書.md](../docs/02_基本設計書.md) — モジュール構成、コマンド仕様
- [03_詳細処理設計書.md](../docs/03_詳細処理設計書.md) — 内部データ構造・処理の詳細設計
- [04_テスト仕様書.md](../docs/04_テスト仕様書.md) — テストケース一覧

## 既知の制限

主なもの（詳細は要件定義書 8 章参照）:

- 既に起動済みの、windbg 自身が生成していないプロセスへのアタッチは非対応（自身が
  `CreateProcess` したプロセスとその子孫のみが対象）
- ブレークポイント・ウォッチポイントはプロセスごとに独立しており、自動伝播しない
- `up` コマンドはローカル変数を持つ関数に対して正しいリターンアドレスを取得できない場合がある
- メモリリーク検出は `malloc`/`calloc`/`realloc`/`free` のみが対象（`new`/`delete` 非対応）、
  かつ CRT を静的リンク（`/MT`/`/MTd`）した対象のみ
- 逆アセンブラは x64 命令の限定サブセットのみ対応
