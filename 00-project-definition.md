# Sift

## What is Sift?

Sift is a tool that looks through records of activity and points out behavior that seems unusual.

Applications record many actions, such as:

- a user logging in;
- a file being opened;
- data being exported;
- a permission being changed;
- a service accessing a resource.

Reading all of these records manually is difficult. Sift processes them automatically and highlights the people, accounts, devices, or services whose behavior changed in a noticeable way.

## Simple example

A user normally exports about four files per day.

One day, the same user exports 81 files, accesses several resources they have never used before, and performs the activity at an unusual time.

Sift reports that this behavior is unusual and explains why:

```text
User: user-142

Why this was flagged:
- 81 files were exported today
- The user's normal daily amount is 4
- 12 previously unseen resources were accessed
- The activity happened at an unusual time
```

Sift does not claim that the user did something malicious. It only shows that the activity is different enough to deserve inspection.

## What goes into Sift?

Sift receives event data from an application.

Each event describes something that happened, such as:

```text
Who performed the action?
What action occurred?
When did it occur?
What resource was involved?
```

## What does Sift produce?

Sift produces a ranked list of unusual activity.

Each result includes:

- who or what behaved unusually;
- how unusual the behavior was;
- what changed;
- the events and measurements that support the result.

## Why build it?

Most audit logs only tell us what happened. They do not tell us which activity is unusual or where an investigator should look first.

Sift turns a large collection of raw events into a smaller list of understandable findings.

## Reference use case

A security analyst reviews audit activity from a collaboration application.

The analyst gives Sift a deterministic synthetic dataset. The dataset contains fictional users and a reproducible mix of login, file-access, sharing, permission-change, and export events. Each event provides at least an event timestamp, an event type, and a primary entity ID.

One user normally exports a small number of files. During one 24-hour period, the user exports substantially more files than their earlier behavior indicates. Sift compares the recent export count with the user's chronological history. It does not use future events when it calculates the historical baseline.

Sift ranks the user for inspection and produces a finding that contains:

- the user ID;
- the anomaly score;
- the recent export count;
- the historical baseline;
- the relevant time range;
- the events that support the result.

The first detector uses export events only. Other event types provide realistic background activity, but they do not affect the first score.

This use case does not include authentication, real-time processing, cohort comparison, or machine-learning detectors. The finding indicates a change in behavior. It does not state that the user acted maliciously.

## One-sentence definition

> Sift analyzes application activity, finds behavior that differs from what is normally expected, and explains why it may deserve human inspection.

## What Sift does not do

Sift does not decide that someone is guilty, malicious, or compromised.

It helps a person decide what should be investigated.
