# RecoverPlan — Implementation notes

These notes describe the source code shipped with the academic MVP, not experimentally validated predictive performance. The Arabic presentation included with the supplied archive informed the conceptual workflow.

## Model inputs

| Model | Number of features | Inputs |
| --- | ---: | --- |
| Duration / linear | 7 | Task difficulty, importance, student's estimated duration, pages/questions, task-type weight, student focus, completion rate |
| Delay / logistic | 8 | Predicted duration (or student estimate), difficulty, importance, days remaining, study hours/day, missed sessions, completion rate, focus |

`TimePredictor` computes `w·x + b`. `DelayPredictor` overrides the prediction method with a sigmoid and uses the inherited batch gradient-update method, which is compatible with the derivative of logistic binary cross-entropy when used with the sigmoid prediction. This implementation does not calculate or display a loss metric or track a training-curve history.

`src/main.cpp` requires at least **three** completed sessions before training the duration predictor, and at least **three** labeled session outcomes before training the delay predictor. These are **demo thresholds**, not scientific recommendations. Before these thresholds, durations fall back to student estimates and delay probabilities remain unavailable (N/A).

## Scheduling

- One-week horizon (days 1–7), with deadlines capped to the horizon.
- Up to the configured number of active sessions per day.
- Maximum duration per session = available hours/day ÷ sessions/day.
- The scheduler supports deadline, importance, difficulty, shortest-duration, and risk prioritization; risk-based scheduling is offered only when the delay model has enough training examples.
- Overflow tasks represent hours that cannot be placed within available daily capacity before the task's deadline.

## Recovery

For a missed session, all planned hours are pending. For a delayed session, the engine records the remaining work according to the existing session representation. Pending work is assigned to subsequent days when capacity exists, subject to the student's daily hours and sessions limits. Low-priority sessions may be shortened to free hours for recovery. An incomplete recovery is explicitly reported rather than silently treated as completed.

## Evaluation not yet performed

To support an AI/ML internship portfolio, the next essential step is a reproducible dataset and evaluation protocol:

1. Create or document a dataset of sessions and outcomes. Keep real and synthetic data clearly separate.
2. Split by student or by time to prevent leakage across repeated sessions.
3. Compare duration prediction against the student's original estimate using MAE and RMSE.
4. Measure delay prediction with a held-out confusion matrix, ROC-AUC, and calibration analysis when there is enough data.
5. Record learning rate, iterations, feature scaling, dataset size, limitations, and repeatable train/test seeds.

No performance or accuracy results are claimed in this repository until those measurements exist.

## Rendering the diagrams

Both flow diagrams are authored as editable Graphviz sources:

```bash
dot -Tsvg docs/diagrams/architecture.dot -o docs/assets/architecture.svg
dot -Tsvg docs/diagrams/recovery.dot -o docs/assets/recovery.svg
```

`docs/assets/sigmoid.svg` is a conceptual plot of the analytic sigmoid equation, not a training result.
