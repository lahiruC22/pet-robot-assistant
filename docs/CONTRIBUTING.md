# Contribution Guidelines

These guidelines are based on the Atlassian documentation for referencing issues in development work. By following these guidelines, we can ensure that our development work is properly linked to our Jira issues, which helps with tracking and project management.

## Referencing Jira Issues

When you are working on a task that has a corresponding Jira issue, you should reference the issue key in your branches, commit messages, and pull requests.

### Jira Issue Key

The Jira issue key (e.g., `PROJ-123`) can be found on the issue card on the board or in the breadcrumb navigation on the issue details page. It is important to use the correct format for the issue key, which is all capital letters. For example, use `PROJ-123` instead of `proj-123`.

### Branch Naming

When you create a new branch for your work, include the Jira issue key in the branch name. This helps to identify the purpose of the branch and automatically links the branch to the Jira issue.

**Example:**
```
git checkout -b PROJ-123-add-new-feature
```

### Commit Messages

Include the Jira issue key in your commit messages. This will link your commits to the development panel in the Jira issue.

**Example:**
```
git commit -m "PROJ-123: Implement the new feature"
```

### Pull Requests

When you create a pull request, include the Jira issue key in the title. This will automatically link the pull request to the Jira issue. If you create the pull request from the development panel in a Jira issue, the key is added automatically.

**Example Pull Request Title:**
```
PROJ-123: Add new feature
```
