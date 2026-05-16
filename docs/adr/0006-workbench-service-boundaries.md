# ADR-0006: Workbench Service Boundaries

## Status

Accepted

## Context

`MainWindow` currently owns shell composition, editor tabs, project tree
actions, search, Problems rows, runtime processes, settings dialogs, and command
palette behavior. That was acceptable for the private beta, but a full IDE needs
features that can be tested and evolved without turning the shell into a single
large coordination point.

## Decision

Keep `MainWindow` as the Qt shell owner, but move behavior into focused services
as each area is touched. The first boundaries are command registry, editor
session/tabs, project operations, search, runtime orchestration, settings, and
language services.

## Consequences

Do not rewrite the workbench in one pass. Each extraction must preserve current
object names, Arabic/RTL behavior, and existing tests. New services should hold
behavior and state; `MainWindow` should compose widgets and connect surfaces.

## Verification

Each extraction needs focused tests for the new service plus unchanged
`acs_main_window_tests` coverage for the visible workbench behavior.
