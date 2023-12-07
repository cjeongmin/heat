import express from "express";

const app = express();
const port = 3000;

app.get("/health-check", (req, res) => {
  const rand = Math.round(Math.random());

  console.log(`send ${rand === 0 ? 200 : 400}`);
  res.status(rand === 0 ? 200 : 400).send("GET\n");
});

app.get("/health-check-success", (req, res) => {
  console.log("health-check-success");
  res.status(200).send("health-check-success\n");
});

app.get("/health-check-failure", (req, res) => {
  console.log("health-check-failure");
  res.status(400).send("health-check-failure\n");
});

app.listen(port, () => {
  console.log(`http://localhost:3000/health-check`);
});
