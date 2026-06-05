# Ravi — Full Progress History & Context Prompt
> Paste this into a new Claude chat to continue from exactly where we left off

## Identity
- **Name:** Ravi
- **Year:** 2nd year CSE + Data Science, Tier 3 college, India
- **OS:** Fedora Linux, zsh, 16GB RAM
- **Editor:** VS Code
- **Package manager:** dnf (not apt)

## Career Goals
- **Primary:** Data Engineering — fresher target 10-18 LPA
- **Secondary:** AI/ML (weekends + Sundays only)

## Project Folders
```
~/Drishti/Sindhu/     → Data Engineering
~/Prajna/             → AI/ML / Kaggle
~/projects/PrajnaOS/  → Bare metal OS kernel
```

---

## SQL Progress

### sqlbolt.com
- Lessons 1–18 — COMPLETE ✅

### LeetCode SQL — 9 problems done
| Problem | Title | Status |
|---|---|---|
| 175 | Combine Two Tables | ✅ |
| 181 | Employees Earning More Than Managers | ✅ |
| 196 | Delete Duplicate Emails | ✅ |
| 197 | Rising Temperature | ✅ |
| 511 | Game Play Analysis I | ✅ |
| 577 | Employee Bonus | ✅ |
| — | Duplicate Emails | ✅ |
| — | Customers No Orders | ✅ |
| — | Second Highest Salary | ✅ |

### Next LeetCode problems
512, 550, 570, 574, 584, 586 — target 25 total before July

### Databases
- **SQLite:** `isro.db` — tables: launchers(77), spacecrafts(119), satellites(122)
- **MariaDB:** installed on Fedora, password reset done, isro.db imported
- **Notes:** `~/Drishti/Sindhu/sql/my_notes.md`

---

## AI/ML Progress

### Book
- Hands-On ML 3rd edition (physical) — Chapter 2

### Kaggle Course
- Intro to ML — Exercise 2 done, Exercise 3 next

### Datasets Worked On

#### 1. ISRO Spacecrafts (119 rows, 7 cols)
- EDA: bar, pie, line charts, year extraction, peak year
- ML: DecisionTree + RandomForest orbit type classifier
- Accuracy: 30% → 63% → **78%** (feature engineering)
- Features used: year, SN, Launch Vehicle, Application, Remarks

#### 2. World GDP per Capita (1990–2020)
- 240 countries, 30 years
- EDA: 4 charts — global trend, top 10, India vs China vs USA, fastest growth
- ML: RandomForestRegressor with lag feature
- RMSE: $18885 → **$1230** (after adding GDP_lag1 feature)
- Predicted 2025: India $5711, China $15295, USA $60409, Afghanistan $2016

#### 3. Airline Delays dataset
- Downloaded — not started yet

### Concepts Learned
- `pd.read_csv`, `describe`, `head`, `isnull`, `dropna`
- `LabelEncoder`, `train_test_split`
- `DecisionTreeClassifier`, `DecisionTreeRegressor`
- `RandomForestClassifier`, `RandomForestRegressor`
- `accuracy_score`, `MAE`, `MSE`, `RMSE`
- Feature engineering — lag features, encoding, extraction
- `cross_val_score`
- `export_text` — export decision tree rules to C code

---

## PrajnaOS Progress

### Completed Levels
- **L1** ✅ Bootloader + VGA
- **L2** ✅ GDT + IDT + Keyboard
- **L3** ✅ Shell + PIT + Colors + Cursor
- **L4** ✅ ATA read/write + FAT32 init + classify command

### Current Bug
- `fat32_find_file()` not matching filenames
- Direct read works — `fat32_read_test()` reads TEST.TXT correctly
- Next: fix name match, then move to L5 memory manager

### Mini Project — IEEE ICACCI
- ML inference in bare metal kernel space — no OS, no stdlib, no Linux
- `classify` shell command → prints "setosa" ✅
- Target: load Iris weights from FAT32, run full inference

---

## DE Roadmap Status
```
Phase 1 (current):
  SQL           ✅ strong
  pandas/numpy  ✅ done
  Linux         ✅ Fedora daily
  Git/GitHub    ❓ needs more activity
  scikit-learn  🔄 in progress

Phase 2 (Sem 5 — next):
  Apache Spark / PySpark
  Airflow (pipelines)
  Cloud — AWS or GCP
  Docker basics
```

---

## Health & Routine
- **Meditation:** midnight sessions (quality declining — phone before bed is cause)
- **Bansuri:** C key, 48cm — practicing Raga Yaman + Raga Bhairav
- **Bhairav time:** morning (5:50–6:50 AM)
- **Yaman time:** evening (4:30–5:00 PM)
- **Meals:** Two-meal Ayurvedic schedule — 10:30 AM + 8 PM
- **Exercise:** Surya Namaskar 10–12 rounds morning

---

## SIH 2026 Plan
- **Project:** PrajnaOS L8 — ML inference in kernel space
- **Angle:** Foundation for India's SHAKTI (RISC-V) processor edge AI
- **Unique:** No SIH team has submitted bare metal ML from scratch
- **NLP interest:** Indian language NLP for future project (Phase 2–3 months away)

---

## Preferences
- Explain everything simply
- Line by line comments in ALL code
- Build step by step
- Connect to ISRO data when possible
- No stdlib in OS code

## Start Here
Ask what I want to work on — SQL, PrajnaOS, ML, or DE concepts.
