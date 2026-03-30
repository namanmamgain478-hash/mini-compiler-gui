const express = require("express");
const bodyParser = require("body-parser");
const cors = require("cors");
const { exec } = require("child_process");
const fs = require("fs");
const path = require("path");

const app = express();
app.use(cors());
app.use(bodyParser.json());

// 🔥 Compile API
app.post("/compile", (req, res) => {
    console.log("Request received");

    const code = req.body.code;

    // file paths
    const filePath = path.join(__dirname, "input.txt");
    const compilerPath = path.join(__dirname, "compiler.exe");
function convertToCustomFormat(code) {
    let lines = code.split("\n");
    let result = [];

    for (let line of lines) {
        line = line.trim();

        if (line === "" || line === "{" || line === "}") continue;

        // 🔥 handle int/float with assignment
        if (line.startsWith("int ") || line.startsWith("float ")) {
            let parts = line.replace(/;/g, "").split("=");

            if (parts.length === 2) {
                let left = parts[0].trim();   // int x
                let right = parts[1].trim();  // 4 * 2

                let varName = left.split(" ")[1];

                result.push(left + ";"); // int x;
                result.push(`${varName} = ${right};`); // x = 4 * 2;
            } else {
                result.push(line);
            }
        }

        else {
            // 🔥 ensure semicolon exists
            if (!line.endsWith(";")) {
                line += ";";
            }
            result.push(line);
        }
    }

    return result.join("\n");
}

  
    // ✅ step 1: code ko file me save karo
const convertedCode = convertToCustomFormat(code);
fs.writeFileSync(filePath, convertedCode);

    // ✅ step 2: compiler run karo (IMPORTANT: quotes for space path)
  exec(`"${compilerPath}" "${filePath}"`, (error, stdout, stderr) => {

    console.log("STDOUT:\n", stdout);
    console.log("STDERR:\n", stderr);

    // 🔥 full output combine
    let finalOutput = stdout + "\n" + stderr;

    if (error) {
        return res.json({
            success: false,
            output: finalOutput || error.message
        });
    }

    res.json({
        success: true,
        output: finalOutput || "No Output"
    });
});
});

// 🔥 Server start
app.listen(5000, () => {
    console.log("🔥 Server running on http://localhost:5000");

       setInterval(() => {}, 1000);
});