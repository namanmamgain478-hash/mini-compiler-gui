function compileCode() {
    const code = document.getElementById("code").value;

    fetch("http://localhost:5000/compile", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({ code: code })
    })
    .then(res => res.json())
   .then(data => {
    const outputBox = document.getElementById("output");
    const output = data.output;

    if (output.includes("=== Errors ===")) {

        const parts = output.split("=== Errors ===");

        const successPart = parts[0];
        const errorPart = "=== Errors ===" + parts[1];

        outputBox.innerHTML = `
            <div class="success-part">${successPart}</div>
            <hr style="border:1px solid #444;">
            <div class="error-part">${errorPart}</div>
        `;

    } else {
        outputBox.innerHTML = `
            <div class="success-part">${output}</div>
        `;
    }
});
}