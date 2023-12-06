import express from "express";

const app = express();
const port = 3000;

app.get("/health-check", (req, res) => {
  const rand = Math.round(Math.random());

  console.log(`send ${rand === 0 ? 200 : 400}`);
  res.status(rand === 0 ? 200 : 400).send("GET");
});

app.listen(port, () => {
  console.log(`listening on port ${port}`);
});
