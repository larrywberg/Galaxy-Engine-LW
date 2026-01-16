#!/usr/bin/env node
"use strict";

const http = require("http");
const fs = require("fs");
const path = require("path");
const url = require("url");

const isRelease = process.argv.includes("--release");
const buildFolder = isRelease ? "wasm-release" : "wasm-debug";
const rootDir = path.resolve(__dirname, "..", "build", buildFolder);
const uiDir = path.resolve(__dirname, "..", "web");
const port = Number(process.env.PORT || 8080);
const cacheDays = Number(process.env.GALAXYENGINE_CACHE_DAYS || 3);
const cacheSeconds = Math.max(0, Math.floor(cacheDays * 24 * 60 * 60));

const mimeTypes = {
  ".html": "text/html; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".wasm": "application/wasm",
  ".css": "text/css; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".png": "image/png",
  ".jpg": "image/jpeg",
  ".jpeg": "image/jpeg",
  ".gif": "image/gif",
  ".svg": "image/svg+xml; charset=utf-8",
  ".txt": "text/plain; charset=utf-8"
};

const cacheableExtensions = new Set([
  ".js",
  ".wasm",
  ".css",
  ".json",
  ".png",
  ".jpg",
  ".jpeg",
  ".gif",
  ".svg",
  ".txt",
  ".data",
  ".bin",
  ".map"
]);

function setCrossOriginHeaders(res) {
  res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
  res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
  res.setHeader("Cross-Origin-Resource-Policy", "same-origin");
}

function setCacheHeaders(res, ext) {
  if (ext === ".html" || cacheSeconds <= 0) {
    res.setHeader("Cache-Control", "no-cache");
    return;
  }
  if (cacheableExtensions.has(ext)) {
    res.setHeader("Cache-Control", `public, max-age=${cacheSeconds}`);
  }
}

const fallbackHtml = `<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <title>GalaxyEngine</title>
    <style>
      html, body { margin: 0; width: 100%; height: 100%; background: #111; }
      #canvas { width: 100%; height: 100%; display: block; }
    </style>
  </head>
  <body>
    <canvas id="canvas"></canvas>
    <script>
      var Module = { canvas: document.getElementById("canvas") };
    </script>
    <script src="GalaxyEngine.js"></script>
  </body>
</html>`;

function safePath(requestPath) {
  const decoded = decodeURIComponent(requestPath);
  const normalized = path.normalize(decoded).replace(/^(\.\.(\/|\\|$))+/, "");
  return path.join(rootDir, normalized);
}

const server = http.createServer((req, res) => {
  setCrossOriginHeaders(res);
  const parsed = url.parse(req.url || "/");
  const requestPath = parsed.pathname || "/";
  const uiRequestPath = requestPath.replace(/^\/+/, "");
  const uiIndex = path.join(uiDir, "index.html");
  const uiAsset = path.join(uiDir, uiRequestPath);
  const filePath = safePath(requestPath === "/" ? "/GalaxyEngine.html" : requestPath);

  fs.stat(uiIndex, (uiErr, uiStats) => {
    if (!uiErr && uiStats.isFile() && requestPath === "/") {
      res.statusCode = 200;
      res.setHeader("Content-Type", "text/html; charset=utf-8");
      res.setHeader("Cache-Control", "no-cache");
      fs.createReadStream(uiIndex).pipe(res);
      return;
    }

    fs.stat(uiAsset, (uiAssetErr, uiAssetStats) => {
      if (!uiAssetErr && uiAssetStats.isFile()) {
        const ext = path.extname(uiAsset);
        res.statusCode = 200;
        res.setHeader("Content-Type", mimeTypes[ext] || "application/octet-stream");
        setCacheHeaders(res, ext);
        fs.createReadStream(uiAsset).pipe(res);
        return;
      }

      fs.stat(filePath, (err, stats) => {
        if (err || !stats.isFile()) {
          if (requestPath === "/" || requestPath === "/GalaxyEngine.html") {
            res.statusCode = 200;
            res.setHeader("Content-Type", "text/html; charset=utf-8");
            res.setHeader("Cache-Control", "no-cache");
            res.end(fallbackHtml);
            return;
          }

          res.statusCode = 404;
          res.setHeader("Content-Type", "text/plain; charset=utf-8");
          res.end("Not found");
          return;
        }

        const ext = path.extname(filePath);
        res.statusCode = 200;
        res.setHeader("Content-Type", mimeTypes[ext] || "application/octet-stream");
        setCacheHeaders(res, ext);
        fs.createReadStream(filePath).pipe(res);
      });
    });
  });
});

server.listen(port, "127.0.0.1", () => {
  const urlText = `http://127.0.0.1:${port}/`;
  console.log(`Serving ${rootDir}`);
  console.log(`Open ${urlText}`);
});
