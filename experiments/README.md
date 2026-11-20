# Reproducing the model checks

I added this experiment to check whether the actual C++ regression classes can learn predictable patterns. This is a **synthetic-data sanity check**, not a test of how well RecoverPlan predicts real students' outcomes.

## Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/model_evaluation
ctest --test-dir build --output-on-failure
```

On Windows with a multi-configuration generator, use `build/Release/model_evaluation.exe` or `ctest --test-dir build -C Release`.

## Setup

- Seed: `20261120`; **no actual student data**.
- Duration: 750 synthetic tasks, with 550 training examples and 200 hold-out cases. We compare linear regression MAE/RMSE against the student's original time estimate.
- Delay: 1,500 synthetic tasks with logistic-generated labels, 1,100 for training and 400 held out. Compare Brier score against a constant training-prevalence baseline and compute ROC-AUC.
- The experiment uses the project's actual C++ models but **experiment-specific learning rates and training iterations**, not necessarily the interactive CLI's hyperparameters.
- The signal-generating equations, data split, and metrics are all in `evaluate_models.cpp`.

See [the measured synthetic results](../results/synthetic_evaluation.md). This only tests implementation on known generated signals.

## Real evaluation still needed

With permission, collect anonymized records containing pre-task estimates, actual duration, task context, and eventual delay labels. **Never upload identifying student data.** Use a split by student or a strictly chronological split to avoid leakage, compare against simple baselines, and report performance and uncertainty on a separate test set.
