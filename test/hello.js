addEventListener("fetch", event => {
  const bug = new Bug();
  console.log("RESULTS", bug.results);
  event.respondWith(new Response());
});
