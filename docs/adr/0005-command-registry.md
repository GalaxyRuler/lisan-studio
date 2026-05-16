# ADR-0005: Command Registry And Action Routing

## Status

Accepted

## Context

The beta shell already exposes commands through top-bar actions, menu rows,
context menus, keyboard shortcuts, and the command palette. As the IDE grows,
duplicating command labels, shortcuts, enablement, and handlers across those
surfaces will make features drift and become hard to test.

## Decision

Introduce a product-owned command registry. User-facing commands have stable
IDs, Arabic labels, categories, default shortcuts, optional descriptions,
enablement checks, and trigger callbacks. Menus, actions, shortcuts, context
menus, tests, and future extensions route through the registry instead of
building separate command lists.

## Consequences

New commands must be registered once before they appear in UI surfaces. Existing
QAction object names remain stable for tests and compatibility, but their
metadata and triggering should come from the registry as each area is migrated.

## Verification

Tests must prove that expected command IDs are registered, the command palette
renders registry metadata, and triggering a palette row calls the same behavior
as the corresponding action or shortcut.
