# Synthetic model evaluation — not real-world accuracy

I ran the C++ predictor classes against generated training and held-out test data. This is a **sanity check using synthetic signals**; it does not measure the models' accuracy on real students.

## Results (fixed seed `20261120`, local Release build)

| Task | Train | Test | Simple baseline | C++ predictor |
| --- | ---: | ---: | --- | --- |
| Task duration | 550 | 200 | MAE **2.4549 h**, RMSE **2.5624 h** (student estimate) | MAE **0.2745 h**, RMSE **0.3418 h** (linear regression) |
| Delay risk | 1100 | 400 | Brier **0.2383** (training-class prevalence) | Brier **0.1896**, ROC-AUC **0.7698** (logistic regression) |

The generator deliberately includes patterns learnable by the models, so these numbers should **not** be described as achieved real-world performance. The experiment uses hyperparameters selected for the synthetic inputs; the interactive MVP uses different defaults. Values may vary slightly by compiler and standard library.

## Reproduce

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/model_evaluation
ctest --test-dir build --output-on-failure
```

The sample generator, split, baselines, and metric calculations are documented in [the experiment](../experiments/evaluate_models.cpp) and [its notes](../experiments/README.md).
