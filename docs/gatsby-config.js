module.exports = {
  siteMetadata: {
    siteTitle: `Lil Data Monster - Loincloth`,
    defaultTitle: `Lil Data Monster - Loincloth`,
    siteTitleShort: `Loincloth`,
    siteDescription: `The Lil Data Monster Loincloth software stack`,
    siteUrl: `https://lildatamonster.github.io/loincloth/`,
    siteAuthor: `David Fan`,
    siteImage: `/banner.png`,
    siteLanguage: `en`,
    themeColor: `#cf2ccb`,
    basePath: `/`,
    footer: `Lil Data Monster`,
  },
  // prefix path for github pages
  pathPrefix: "/loincloth",
  plugins: [
    {
      resolve: `@rocketseat/gatsby-theme-docs`,
      options: {
        configPath: `src/config`,
        docsPath: `src/docs`,
        githubUrl: `https://github.com/LilDataMonster/loincloth`,
        baseDir: `docs`,
      },
    },
    {
      resolve: `gatsby-plugin-manifest`,
      options: {
        name: `Loincloth`,
        short_name: `Loincloth`,
        start_url: `/`,
        background_color: `#ffffff`,
        display: `standalone`,
        icon: `static/favicon.png`,
      },
    },
    `gatsby-plugin-sitemap`,
    {
      resolve: `gatsby-plugin-google-analytics`,
      options: {
        // trackingId: ``,
      },
    },
    {
      resolve: `gatsby-plugin-canonical-urls`,
      options: {
        siteUrl: `https://lildatamonster.github.io/loincloth/`,
      },
    },
    `gatsby-plugin-offline`,
  ],
};
