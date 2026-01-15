#!/usr/bin/env node
"use strict";

const { spawnSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const args = process.argv.slice(2);
const useDebug = args.includes("--debug");
const useDelete = args.includes("--delete");
const dryRun = args.includes("--dry-run");

const buildFolder = useDebug ? "wasm-debug" : "wasm-release";
const rootDir = path.resolve(__dirname, "..");
const buildDir = path.join(rootDir, "build", buildFolder);
const webDir = path.join(rootDir, "web");

const defaultTarget = "dh_sm759m@physics.artengines.com:/home/dh_sm759m/physics.artengines.com/";
const deployTargetRaw = process.env.GALAXY_ENGINE_DEPLOY_TARGET || defaultTarget;
const deployTarget = deployTargetRaw.endsWith("/") ? deployTargetRaw : `${deployTargetRaw}/`;

function ensureDirExists(dirPath, label) {
  if (!fs.existsSync(dirPath)) {
    console.error(`[deploy] Missing ${label} directory: ${dirPath}`);
    process.exit(1);
  }
}

function ensureFileExists(filePath) {
  if (!fs.existsSync(filePath)) {
    console.error(`[deploy] Missing build output: ${filePath}`);
    process.exit(1);
  }
}

ensureDirExists(webDir, "web");
ensureDirExists(buildDir, "build");

[
  "GalaxyEngine.js",
  "GalaxyEngine.wasm",
  "GalaxyEngine.data",
  "GalaxyEngine.worker.js"
].forEach((file) => ensureFileExists(path.join(buildDir, file)));

const baseArgs = ["-avz", "--progress"];
if (useDelete) baseArgs.push("--delete");
if (dryRun) baseArgs.push("--dry-run");

function runRsync(extraArgs, label) {
  const result = spawnSync("rsync", [...baseArgs, ...extraArgs], {
    stdio: "inherit"
  });

  if (result.error) {
    if (result.error.code === "ENOENT") {
      console.error("[deploy] rsync not found. Install rsync or use scp instead.");
    } else {
      console.error(`[deploy] ${label} failed: ${result.error.message}`);
    }
    process.exit(1);
  }

  if (result.status !== 0) {
    console.error(`[deploy] ${label} failed with exit code ${result.status}`);
    process.exit(result.status);
  }
}

console.log(`[deploy] Target: ${deployTarget}`);
console.log(`[deploy] Build: ${buildFolder}`);

runRsync([`${webDir}${path.sep}`, deployTarget], "web sync");
runRsync(
  ["--include=GalaxyEngine.*", "--exclude=*", `${buildDir}${path.sep}`, deployTarget],
  "wasm sync"
);

console.log("[deploy] Done.");
