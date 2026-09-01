# Development Plan: Adding Pentax Support to libgphoto2 Fork

> **OBSOLETE:** Historical draft only. Do not implement from this file. Use the canonical [DEVELOPMENT_PLAN.md](DEVELOPMENT_PLAN.md), revision 2026-08-21.

This document specifies exactly what changes to make to the user's fork of libgphoto2 (ian-morgan99/libgphoto2) to implement full Pentax camera support based on analysis of Pentax Image Transmitter 2. The plan is broken into incremental, testable stages.

## Prerequisites
- Working libgphoto2 build environment (meson/autotools)
- Access to Pentax K-3 III and/or K-1 II hardware for validation
- Base fork should start from the latest stable release of libgphoto2. If specific fixes from the main branch are required, document and verify their stability before use.
- Local copy of Image Transmitter 2 source available for reference

## Overview
We will implement support in two parts:
1. **Part 1: Basic Generic PTP Support** - Provides parity with upstream Monochrome K-3 III (PR #1273): detection, configuration, file download.
2. **Part 2: Pentax-Specific Extensions** - Adds live view, advanced capture control, extensive configuration, model-specific gating, and status feedback.

Each part is divided into stages with clear entrance/exit criteria and specific code changes.
