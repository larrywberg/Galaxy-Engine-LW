#!/usr/bin/env node
"use strict";

const http = require("http");
const fs = require("fs");
const path = require("path");
const url = require("url");

const rootDir = path.resolve(__dirname, "..", "build", "wasm-debug");
const port = Number(process.env.PORT || 8080);

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
  const parsed = url.parse(req.url || "/");
  const requestPath = parsed.pathname || "/";
  const filePath = safePath(requestPath === "/" ? "/GalaxyEngine.html" : requestPath);

  fs.stat(filePath, (err, stats) => {
    if (err || !stats.isFile()) {
      if (requestPath === "/" || requestPath === "/GalaxyEngine.html") {
        res.statusCode = 200;
        res.setHeader("Content-Type", "text/html; charset=utf-8");
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
    fs.createReadStream(filePath).pipe(res);
  });
});

server.listen(port, "127.0.0.1", () => {
  const urlText = `http://127.0.0.1:${port}/GalaxyEngine.html`;
  console.log(`Serving ${rootDir}`);
  console.log(`Open ${urlText}`);
});
